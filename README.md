# Rust Qt Bindings Generator

![Rust Qt Bindings](logo.svg)

This generator gets you started quickly to use Rust code from Qt and QML.

The building block of Qt and QML projects are QObject and the Model View classes. `rust_qt_binding_generator` reads a json file to generate QObject or QAbstractItemModel classes that call into generated Rust files. For each type from the JSON file, a Rust trait is generated that should be implemented.

This way, Rust code can be called from Qt and QML projects.
