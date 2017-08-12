# QR Demo for Vita by cxziaho  
Decode QR codes from the camera, in realtime.  
Uses the [QUIRC](https://github.com/dlbeer/quirc) library, which I ported to Vita [here](https://github.com/cxziaho/quirc-vita), and uses [code](https://github.com/Rinnegatamante/lpp-vita/blob/master/source/luaCamera.cpp) from LuaPlayer Plus to init camera.
This is a PoC.  Hopefully people can make some cool stuff using this library.  
  
I might fork the VitaShell library and implement this, as my attempt at a standalone version of this wasn't working.  
    
## Building   
Assuming you have the [VitaSDK](http://vitasdk.org) toolchain:  
```  
git clone https://github.com/cxziaho/qrdemo.git  
cd qrdemo  
cmake
make  
```
and use `qrdemo.vpk` in the folder.  
  
## Notes
In this example I use multithreading to remove lag caused (most likely) by this code:  
```
colourRGBA = qr_data[(y*CAM_WIDTH)+x];
red = (colourRGBA & 0x000000FF);
green = (colourRGBA & 0x0000FF00) >> 8;
blue = (colourRGBA & 0x00FF0000) >> 16;
image[(y*CAM_WIDTH)+x] = (red + green + blue) / 3;
```
If you can fix this issue; please submit a pull request!  I would love for it to all work in one thread.