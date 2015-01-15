Building MiniServo-GTK
----------------------

1) Clone Servo and MiniServo so your directory structure looks like:

+ servo             (https://github.com/servo/servo)
+ miniservo-gtk     (https://github.com/glennw/miniservo-gtk)

2) Build CEF libembedding

cd servo && ./mach build-cef

3) Build MiniServo-GTK

cd miniservo-gtk
mkdir build && cd build
cmake ..
make -j16

4) Run MiniServo

./miniservo
