# Home Assistant bridge

The examples connect two MQTT interfaces:

- `sdpro/display/call_lift` receives `PRESS` from the clock;
- `sdpro/display/lift/*` sends normalized floor, direction and state to it.

Replace the example entity IDs with the entities created by your recognizer and
ESPHome relay. Never copy a complete `.storage` directory into a public repo;
it contains tokens, device keys, private addresses and account data.

The relay configuration exposes a momentary `button` entity. Calling that entity
is safer than exposing a latching relay switch to dashboards and automations.
