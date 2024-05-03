<h1>
Necessity for Thread Locks in Matrix Multiplications with Atomic Data
</h1>

One of the main benefits with working with ```std::atomic``` data is to provide thread safety without having to use thread locks. This repository contains proof of concept code that without any thread locks, matrix multiplication can yield wrong results.

## 📜 LICENSE
This repository is under the [MIT License](./LICENSE)