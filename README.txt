Building MiniServo-GTK
----------------------

There's currently several submodules that needed minor changes, which makes it 
more complex to build at the moment. These should be merged into master in the
near future which will simplify things.

1) Clone https://github.com/glennw/rust-harfbuzz and checkout cef branch
2) Clone https://github.com/glennw/rust-png and checkout cef branch
3) Clone https://github.com/glennw/miniservo-gtk

Build CEF libembedding (you need to setup .cargo/config to use rust-harfbuzz and
rust-png for now).

./mach build-cef

Build miniservo:

mkdir build && cd build
cmake ..
make -j16

Run miniservo:

export LD_LIBRARY_PATH=<path to libembedding.so built above>
./miniservo
