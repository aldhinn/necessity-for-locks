<h1>
Necessity for Thread Locks in Matrix Operations
</h1>

One of the main benefits with working with ```std::atomic``` data is to provide thread safety without having to use thread locks. This repository contains proof of concept code that without any thread locks, some matrix operations can yield wrong results.

## 📜 LICENSE
This repository is under the [MIT License](./LICENSE)