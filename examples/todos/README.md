Source code for https://fosdem.org/2018/schedule/event/rust_qt_binding_generator/

Make sure `rust_qt_binding_generator` is in the `PATH`.

```bash
mkdir build
cd build
cmake -GNinja ..
ninja
export QT_QUICK_CONTROLS_MATERIAL_THEME=Dark
export QT_QUICK_CONTROLS_STYLE=Material
./todos
```
