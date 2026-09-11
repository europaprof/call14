"""Small translation registration using stationary metal, never LED digits."""
import json
import numpy as np
from PIL import Image


class PanelAlignment:
    def __init__(self, path):
        model=json.loads(path.read_text())
        reference=np.asarray(model['gray'],dtype=np.float32)
        # Metal texture outside the display is independent of floor and arrows.
        yy,xx=np.mgrid[12:76:2,5:110:2]
        mask=(xx<17)|(xx>91)
        self.x=xx[mask]; self.y=yy[mask]
        self.offsets=[(dx,dy) for dy in range(-8,11) for dx in range(-3,4)]
        self.xs=np.array([self.x+dx for dx,dy in self.offsets])
        self.ys=np.array([self.y+dy for dx,dy in self.offsets])
        self.reference=self.features(reference,self.y,self.x)
        self.reference-=self.reference.mean()
        self.reference/=np.linalg.norm(self.reference)+1e-6
        self.last=(0,0)
        self.quality=0.0

    @staticmethod
    def features(gray,y,x):
        return np.concatenate((gray[y,x+1]-gray[y,x-1],gray[y+1,x]-gray[y-1,x]),axis=-1)

    def apply(self,image):
        gray=np.asarray(image.convert('L'),dtype=np.float32)
        features=self.features(gray,self.ys,self.xs)
        features-=features.mean(axis=1,keepdims=True)
        features/=np.linalg.norm(features,axis=1,keepdims=True)+1e-6
        scores=features@self.reference
        best=int(np.argmax(scores)); self.quality=float(scores[best])
        if self.quality>=0.45:
            self.last=self.offsets[best]
        dx,dy=self.last
        return image.transform(image.size,Image.Transform.AFFINE,(1,0,dx,0,1,dy))
