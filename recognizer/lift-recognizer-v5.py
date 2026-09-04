#!/usr/bin/env python3
import io
import json
import math
import os
import signal
import subprocess
import sys
import time
from collections import deque
from datetime import datetime, timezone
from pathlib import Path

from PIL import Image, ImageChops

RTSP_URL = os.environ['LIFT_RTSP_URL']
TEMPLATES_PATH = Path(os.getenv('LIFT_TEMPLATES', '/opt/lift-recognizer/templates-v4.json'))
SEGMENT_MODEL_PATH = Path(os.getenv('LIFT_SEGMENT_MODEL', '/opt/lift-recognizer/segment-v5.json'))
MQTT_HOST = os.getenv('LIFT_MQTT_HOST', '127.0.0.1')
MQTT_PORT = os.getenv('LIFT_MQTT_PORT', '1883')
MQTT_USER = os.getenv('LIFT_MQTT_USER', '')
MQTT_PASSWORD = os.getenv('LIFT_MQTT_PASSWORD', '')
MQTT_PREFIX = os.getenv('LIFT_MQTT_PREFIX', 'building/lift/1/v4_shadow')
DISCOVERY_PREFIX = os.getenv('LIFT_DISCOVERY_PREFIX', 'homeassistant')
DEVICE_ID = os.getenv('LIFT_DEVICE_ID', 'lift_1_passenger')
DEVICE_NAME = os.getenv('LIFT_DEVICE_NAME', 'Лифт 1 Пассажирский')
OBJECT_PREFIX = os.getenv('LIFT_OBJECT_PREFIX', DEVICE_ID)
FRAME_CROP = os.getenv('LIFT_FRAME_CROP', '120:95:340:0')
FRAME_RATE = float(os.getenv('LIFT_FRAME_RATE', '4'))
HWACCEL = os.getenv('LIFT_HWACCEL', '').strip().lower()
VAAPI_DEVICE = os.getenv('LIFT_VAAPI_DEVICE', '/dev/dri/renderD128')
LEFT_THRESHOLD = float(os.getenv('LIFT_LEFT_THRESHOLD', '65'))
RIGHT_THRESHOLD = float(os.getenv('LIFT_RIGHT_THRESHOLD', '50'))
VALID_FLOORS = [int(x) for x in os.getenv(
    'LIFT_VALID_FLOORS', '1,4,5,6,7,8,9,10,11,12,13,14,15,16'
).split(',')]

def env_box(name, default):
    return tuple(int(x) for x in os.getenv(name, ','.join(map(str, default))).split(','))

DIGIT_BOX = env_box('LIFT_DIGIT_BOX', (37, 2, 71, 64))
LEFT_ARROW_BOX = env_box('LIFT_LEFT_ARROW_BOX', (19, 28, 34, 78))
RIGHT_ARROW_BOX = env_box('LIFT_RIGHT_ARROW_BOX', (72, 5, 82, 58))

RUNNING = True

def stop(*_):
    global RUNNING
    RUNNING = False

signal.signal(signal.SIGTERM, stop)
signal.signal(signal.SIGINT, stop)

def now_iso():
    return datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')

def mqtt_publish(topic, payload, retain=True):
    cmd = ['mosquitto_pub', '-h', MQTT_HOST, '-p', MQTT_PORT, '-t', topic]
    if MQTT_USER: cmd += ['-u', MQTT_USER]
    if MQTT_PASSWORD: cmd += ['-P', MQTT_PASSWORD]
    if retain:
        cmd.append('-r')
    cmd += ['-m', payload if isinstance(payload, str) else json.dumps(payload, ensure_ascii=False, separators=(',', ':'))]
    subprocess.run(cmd, check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def publish_discovery():
    device = {'identifiers': [DEVICE_ID], 'name': DEVICE_NAME,
              'manufacturer': 'Local RTSP recognizer', 'model': 'Seven-segment camera sensor'}
    state_topic = f'{MQTT_PREFIX}/state'
    availability_topic = f'{MQTT_PREFIX}/availability'
    entities = {
        ('sensor','floor'): {'name':'Этаж','icon':'mdi:elevator','value_template':'{{ value_json.floor if value_json.floor is not none else "unknown" }}'},
        ('sensor','state'): {'name':'Состояние','icon':'mdi:elevator-passenger','value_template':'{{ value_json.state }}'},
        ('sensor','direction'): {'name':'Направление','icon':'mdi:swap-vertical','value_template':'{{ value_json.direction }}'},
        ('sensor','display'): {'name':'Табло','icon':'mdi:monitor','value_template':'{{ value_json.display }}'},
        ('sensor','confidence'): {'name':'Уверенность','icon':'mdi:check-decagram','unit_of_measurement':'%','value_template':'{{ (value_json.confidence * 100) | round(0) }}'},
        ('sensor','floor_source'): {'name':'Источник этажа','icon':'mdi:source-branch','value_template':'{{ value_json.floor_source }}'},
        ('sensor','last_seen'): {'name':'Последний кадр','device_class':'timestamp','value_template':'{{ value_json.last_seen }}'},
        ('binary_sensor','moving'): {'name':'Движется','icon':'mdi:elevator-passenger','payload_on':'ON','payload_off':'OFF','value_template':'{{ "ON" if value_json.moving else "OFF" }}'},
        ('binary_sensor','fault'): {'name':'Неисправность','device_class':'problem','payload_on':'ON','payload_off':'OFF','value_template':'{{ "ON" if value_json.fault else "OFF" }}'},
        ('binary_sensor','camera'): {'name':'Камера доступна','device_class':'connectivity','payload_on':'ON','payload_off':'OFF','value_template':'{{ "ON" if value_json.camera_online else "OFF" }}'},
        ('binary_sensor','estimated'): {'name':'Этаж прогнозируется','icon':'mdi:timer-sand','payload_on':'ON','payload_off':'OFF','value_template':'{{ "ON" if value_json.estimated else "OFF" }}'},
    }
    for (component, key), extra in entities.items():
        cfg = {'unique_id':f'{DEVICE_ID}_{key}', 'object_id':f'{OBJECT_PREFIX}_{key}', 'state_topic':state_topic,
               'availability_topic':availability_topic, 'payload_available':'online', 'payload_not_available':'offline',
               'device':device, **extra}
        mqtt_publish(f'{DISCOVERY_PREFIX}/{component}/{DEVICE_ID}_{key}/config', cfg)

def normalized_digit_vector(display):
    digits = display.crop(DIGIT_BOX).resize((25, 31), Image.Resampling.LANCZOS)
    vals = [float(g) for _, g, _ in digits.getdata()]
    mean = sum(vals) / len(vals)
    std = math.sqrt(sum((v-mean)**2 for v in vals) / len(vals)) or 1.0
    return [(v-mean)/std for v in vals]

def distance(a, b):
    return sum((x-y)**2 for x, y in zip(a,b))/len(a)

def arrow_feature(display, box):
    vals = [min(g,b) for r,g,b in display.crop(box).getdata()]
    return sum(vals)/len(vals)

def digit_activity(display):
    vals = sorted((min(g,b) for r,g,b in display.crop(DIGIT_BOX).getdata()), reverse=True)
    count = max(1, len(vals)//10)
    return sum(vals[:count])/count

def dash_like(display):
    crop = display.crop(DIGIT_BOX)
    def bright_fraction(box):
        vals=[min(g,b)>115 for r,g,b in crop.crop(box).getdata()]
        return sum(vals)/len(vals)
    middle=bright_fraction((0,25,34,38))
    vertical=(bright_fraction((0,5,8,57))+bright_fraction((26,5,34,57)))/2
    return middle > 0.13 and vertical < 0.08

class Recognizer:
    def __init__(self):
        data=json.loads(TEMPLATES_PATH.read_text())
        self.templates=data['items']
        self.segment_model=json.loads(SEGMENT_MODEL_PATH.read_text()) if SEGMENT_MODEL_PATH.exists() else None
        self.frames=deque(maxlen=3)
        self.frame_count=0
        self.floor=None
        self.confirmed_floor=None
        self.estimated=False
        self.prediction_steps=0
        self.last_progress=time.monotonic()
        self.direction='none'
        self.last_motion_direction=None
        self.idle_frames=999
        self.arrow_misses=0
        self.state='unknown'
        self.candidate=None
        self.candidate_count=0
        self.bad_since=None
        self.unreadable_since=None
        self.fault_count=0
        self.last_change=now_iso()
        self.last_publish=0.0
        self.last_payload=None
        self.all_floor_cache=(None,1.0,0.0,0.0)
        self.resync_candidate=None
        self.resync_count=0

    def classify_segments(self, display):
        if not self.segment_model:
            return None, 9, 0.0, ''
        _,green,blue=display.split()
        signal=ImageChops.darker(green,blue)
        pixels=signal.load()
        reference=sorted(pixels[x,y] for y in range(2,64) for x in range(37,71))
        scale=max(1.0,reference[int(len(reference)*0.95)])
        def score(mask):
            return sum(pixels[x,y]/scale for x,y in mask)/len(mask)
        active=set(); evidence=[]
        for name in 'abcdefg':
            item=self.segment_model['segments'][name]
            value=score(item['mask']); delta=value-item['threshold']
            evidence.append(abs(delta))
            if delta>=0: active.add(name)
        patterns={0:set('abcdef'),1:set('bc'),2:set('abdeg'),3:set('abcdg'),
                  4:set('bcfg'),5:set('acdfg'),6:set('acdefg'),7:set('abc'),
                  8:set('abcdefg'),9:set('abcdfg')}
        ranked=sorted((len(active^pattern),digit) for digit,pattern in patterns.items())
        errors,ones=ranked[0]
        tens=self.segment_model['tens']; tens_score=score(tens['mask'])
        floor=10+ones if tens_score>=tens['threshold'] else ones
        quality=min(evidence) if evidence else 0.0
        if floor not in VALID_FLOORS or errors:
            floor=None
        return floor,errors,quality,''.join(sorted(active))

    def allowed_floors(self, direction):
        if self.confirmed_floor not in VALID_FLOORS:
            return set(VALID_FLOORS)
        # With no illuminated arrow the car is stationary.  Door/cabin lighting must
        # never be allowed to rewrite an already confirmed floor.
        if direction not in ('up','down'):
            allowed={self.confirmed_floor}
            # When the arrow goes dark at arrival, allow only the immediately
            # adjacent floor in the just-finished direction for five seconds.
            # This anchors floor 1 after 4, without letting door light rewrite
            # an already settled car later.
            if self.idle_frames < 20 and self.last_motion_direction in ('up','down'):
                index=VALID_FLOORS.index(self.confirmed_floor)
                step=1 if self.last_motion_direction=='up' else -1
                if 0 <= index+step < len(VALID_FLOORS):
                    allowed.add(VALID_FLOORS[index+step])
            return allowed
        index=VALID_FLOORS.index(self.confirmed_floor)
        step=1 if direction=='up' else -1
        allowed={self.confirmed_floor}
        if 0 <= index+step < len(VALID_FLOORS): allowed.add(VALID_FLOORS[index+step])
        return allowed

    def classify_floor(self, display, direction, allowed_override=None):
        sample=normalized_digit_vector(display)
        allowed=self.allowed_floors(direction) if allowed_override is None else set(allowed_override)
        # Arrow lighting changes the appearance of the slanted red panel.
        # Never compare an up/down/idle frame with templates from another mode.
        eligible=[t for t in self.templates
                  if t['floor'] in allowed and t.get('direction', direction)==direction]
        if direction=='down' and 1 in allowed:
            eligible += [t for t in self.templates if t['floor']==1 and t not in eligible]
        if not eligible:
            eligible=[t for t in self.templates if t['floor'] in allowed]
        ranked=sorted((distance(sample,t['vector']),t['floor']) for t in eligible)
        best_distance,best_floor=ranked[0]
        other=next((d for d,f in ranked if f!=best_floor),1.0)
        margin=other-best_distance
        confidence=max(0.0,min(1.0,margin/0.18))
        return best_floor,best_distance,margin,confidence

    def physically_valid(self, floor, direction):
        if self.confirmed_floor is None or floor==self.confirmed_floor:
            return True
        try:
            old=VALID_FLOORS.index(self.confirmed_floor); new=VALID_FLOORS.index(floor)
        except ValueError:
            return False
        delta=new-old
        if direction=='up' and delta<0: return False
        if direction=='down' and delta>0: return False
        # Frames may skip intermediate floors; monotonic direction is the invariant.
        return True

    def process(self, frame):
        self.frames.append(frame)
        self.frame_count+=1
        # Quality first: classify all 4 fps; two confirmations take about 0.5 s.
        if len(self.frames)<3: return
        # Never merge adjacent floor numbers: classify the middle frame and vote over time.
        display=self.frames[1]
        arrow_display=self.frames[0]
        for image in list(self.frames)[1:]:
            arrow_display=ImageChops.lighter(arrow_display,image)

        left=arrow_feature(arrow_display,LEFT_ARROW_BOX)
        right=arrow_feature(arrow_display,RIGHT_ARROW_BOX)
        # The LED arrows are multiplexed.  A camera frame can catch a dark
        # scan phase even while the car is moving, especially for the smaller
        # right/down arrow.  Start immediately on positive evidence, but only
        # stop after eight consecutive misses (~2 seconds at 4 fps).
        if left>LEFT_THRESHOLD and right<42: raw_direction='up'
        elif right>42 and left<LEFT_THRESHOLD: raw_direction='down'
        elif left>LEFT_THRESHOLD and right>42: raw_direction='unknown'
        else: raw_direction='none'
        if raw_direction in ('up','down'):
            direction=raw_direction
            self.arrow_misses=0
        elif self.direction in ('up','down') and self.arrow_misses<4:
            self.arrow_misses+=1
            direction=self.direction
        else:
            self.arrow_misses+=1
            direction='none'
        if direction in ('up','down'):
            self.last_motion_direction=direction
            self.idle_frames=0
        else:
            self.idle_frames+=1
        now=time.monotonic()
        if direction!=self.direction:
            self.last_progress=now
            self.prediction_steps=0
            if direction in ('up','down') and self.confirmed_floor is not None:
                self.floor=self.confirmed_floor
                self.estimated=False

        floor,dist,margin,confidence=self.classify_floor(display,direction)
        segment_floor,segment_errors,segment_quality,segments=self.classify_segments(arrow_display)
        allowed_now=self.allowed_floors(direction)
        segment_reliable=(segment_floor in allowed_now and segment_errors==0 and segment_quality>=0.025)
        template_reliable=(dist<0.68 and margin>0.004 and floor in VALID_FLOORS)
        # Segments are primary when their seven-bit code is clean and physically
        # possible.  Otherwise retain the proven directional template fallback.
        if segment_reliable:
            floor=segment_floor
            confidence=max(confidence,min(1.0,segment_quality/0.10))
        if self.frame_count % 16 == 0 or self.confirmed_floor is None:
            self.all_floor_cache=self.classify_floor(display,direction,VALID_FLOORS)
        all_floor,all_dist,all_margin,all_confidence=self.all_floor_cache
        activity=digit_activity(display)
        # Adjacency is the primary guard; require a modest image match and three votes.
        reliable=segment_reliable or template_reliable
        resync_ok=(direction=='none' and activity>150 and segment_errors==0 and
                   segment_quality>=0.05 and segment_floor==all_floor and
                   all_floor in VALID_FLOORS and all_dist<0.30 and all_margin>0.01)
        if resync_ok and all_floor!=self.confirmed_floor:
            if self.resync_candidate==all_floor:
                self.resync_count+=1
            else:
                self.resync_candidate=all_floor
                self.resync_count=1
            # Three seconds of agreement between two independent decoders.
            if self.resync_count>=12:
                floor=all_floor
                reliable=True
        else:
            self.resync_candidate=None
            self.resync_count=0
        display_blank=activity<35 and dist>0.50 and direction=='none'

        if dash_like(display) and not reliable:
            self.fault_count+=1
        else:
            self.fault_count=0

        if display_blank:
            if self.bad_since is None: self.bad_since=time.monotonic()
        else:
            self.bad_since=None

        accepted_geometry=reliable and self.physically_valid(floor,direction)
        if accepted_geometry:
            self.unreadable_since=None
            if floor==self.candidate: self.candidate_count+=1
            else: self.candidate=floor; self.candidate_count=1
            confirmations=2 if direction in ('up','down') else (5 if self.confirmed_floor is None else 4)
            if self.candidate_count>=confirmations and (floor!=self.confirmed_floor or self.estimated):
                self.confirmed_floor=floor
                self.floor=floor
                self.estimated=False
                self.prediction_steps=0
                self.last_progress=now
                self.last_change=now_iso()
        else:
            if self.unreadable_since is None: self.unreadable_since=time.monotonic()
            confidence=0.0

        bad_for=0 if self.bad_since is None else time.monotonic()-self.bad_since
        fault=self.fault_count>=5
        if fault:
            state='fault'; display_text='--'
        elif bad_for>=30:
            state='display_off'; display_text='OFF'
        elif direction=='up':
            state='moving_up'; display_text=str(self.floor) if self.floor is not None else '?'
        elif direction=='down':
            state='moving_down'; display_text=str(self.floor) if self.floor is not None else '?'
        else:
            state='idle'; display_text=str(self.floor) if self.floor is not None else '?'

        if state!=self.state or direction!=self.direction:
            self.last_change=now_iso()
        self.state=state; self.direction=direction
        payload={'floor':self.floor,'state':state,'direction':direction,
                 'moving':direction in ('up','down'),'display':display_text,'fault':fault,
                 'camera_online':True,'confidence':round(confidence,3),'last_seen':now_iso(),
                 'confirmed_floor':self.confirmed_floor,'floor_source':'estimated' if self.estimated else 'camera',
                 'estimated':self.estimated,'prediction_steps':self.prediction_steps,
                 'last_changed':self.last_change,'left_level':round(left,1),'right_level':round(right,1),
                 'distance':round(dist,3),'margin':round(margin,3),'activity':round(activity,1),'source':'hybrid_segment_v5',
                 'segment_floor':segment_floor,'segment_errors':segment_errors,
                 'segment_quality':round(segment_quality,3),'segments':segments}
        # Unrestricted classification is diagnostic only. It must never rewrite a
        # confirmed stationary floor: door/cabin light made floor 1 resemble 5 in
        # production and proved that stationary re-sync is unsafe.
        payload.update({'all_floor':all_floor,'all_distance':round(all_dist,3),
                        'all_margin':round(all_margin,3)})
        self.publish(payload)

    def publish(self,payload,force=False):
        now=time.monotonic()
        diagnostics=('last_seen','confidence','left_level','right_level','distance','margin','activity',
                     'all_floor','all_distance','all_margin')
        material={k:v for k,v in payload.items() if k not in diagnostics}
        previous=None if self.last_payload is None else {k:v for k,v in self.last_payload.items() if k not in diagnostics}
        if force or material!=previous or now-self.last_publish>=30:
            mqtt_publish(f'{MQTT_PREFIX}/state',payload)
            mqtt_publish(f'{MQTT_PREFIX}/floor',str(payload['floor'] or 'unknown'))
            mqtt_publish(f'{MQTT_PREFIX}/direction',payload['direction'])
            mqtt_publish(f'{MQTT_PREFIX}/display',payload['display'])
            self.last_payload=payload; self.last_publish=now
            print(json.dumps(payload,ensure_ascii=False),flush=True)

def frame_stream():
    cmd=['ffmpeg','-nostdin','-hide_banner','-loglevel','error']
    filters=[]
    if HWACCEL == 'vaapi':
        cmd += ['-hwaccel','vaapi','-hwaccel_device',VAAPI_DEVICE,
                '-hwaccel_output_format','vaapi']
        filters += ['hwdownload','format=nv12']
    filters += [f'fps={FRAME_RATE:g}',f'crop={FRAME_CROP}']
    cmd += ['-rtsp_transport','tcp','-i',RTSP_URL,
         '-vf',','.join(filters),
         '-q:v','3','-f','image2pipe','-vcodec','mjpeg','pipe:1']
    process=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,bufsize=0)
    data=bytearray()
    try:
        while RUNNING:
            chunk=process.stdout.read(4096)
            if not chunk: break
            data.extend(chunk)
            while True:
                start=data.find(b'\xff\xd8'); end=data.find(b'\xff\xd9',start+2)
                if start<0 or end<0: break
                jpg=bytes(data[start:end+2]); del data[:end+2]
                try: yield Image.open(io.BytesIO(jpg)).convert('RGB')
                except Exception: pass
    finally:
        process.terminate()
        try: process.wait(timeout=3)
        except subprocess.TimeoutExpired: process.kill()

def main():
    publish_discovery()
    mqtt_publish(f'{MQTT_PREFIX}/availability','online')
    recognizer=Recognizer()
    retry=1
    while RUNNING:
        last_frame=time.monotonic()
        try:
            for frame in frame_stream():
                last_frame=time.monotonic(); retry=1; recognizer.process(frame)
                if not RUNNING: break
        except Exception as exc:
            print(f'stream error: {exc}',file=sys.stderr,flush=True)
        if not RUNNING: break
        offline={'floor':recognizer.floor,'state':'camera_offline','direction':'unknown','moving':False,
                 'display':'CAM','fault':False,'camera_online':False,'confidence':0.0,
                 'last_seen':now_iso(),'last_changed':now_iso(),'source':'rtsp_main_crop'}
        recognizer.publish(offline,force=True)
        time.sleep(retry); retry=min(30,retry*2)
    mqtt_publish(f'{MQTT_PREFIX}/availability','offline')

if __name__=='__main__': main()
