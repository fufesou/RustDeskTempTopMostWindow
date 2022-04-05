# RustDesk Temporation TopMost Window

A temporary solution of privacy protection window on Win10. The privacy protection window is a window that covers all other windows besides taskbar and start menu.

This project is derived from [MobileShell](https://github.com/ADeltaX/MobileShell). The explanation blog can be found [here](https://blog.adeltax.com/window-z-order-in-windows-10/).

As DLL injection is used, this sulution is not good enough and will be replaced in the future.

By default, this project will inject "C:\windows\System32\RuntimeBroker.exe".


**NOTE**

Run demo will cause the whole screen to be covered.
It will be better to set ```test``` to be true in line ```bool test = false;``` [dllmain.cpp](./WindowInjection/dllmain.cpp).

Run ```taskkill /F /IM MiniBroker.exe``` to close the white window.
