# yuv2fb - YUV to framebuffer

Displays a YUV frame, stored in a file, in the /dev/fb0 device.

Should be run from a non-X screen (Ctrl-Alt-F1, etc).

YUV files are created by `h264dec` utility, built in `github.com/cisco/openh264`. To split a YUV file into individual frames, use `split --bytes=<frame-size> test.yuv` where `frame-size` is width\*height\*1.5.
