# Cache Memory Simulator in C++

This project is a **cache memory simulator** written in C++, it was developed as part of a assignment at Universidade de Caxias do Sul(UCS). It models a simplified CPU cache using configurable parameters such as write policy, replacement policy, associativity, and line size. The simulator processes memory access traces and computes performance metrics like hit/miss rates and average access time.

---

## Features

- **Write Policies**:
  - `Write-through`
  - `Write-back`
- **Replacement Policies**:
  - `Random`
  - `Least Recently Used (LRU)`
- **Associativity**: Configurable (direct-mapped, N-way set associative)
- **Performance Metrics**:
  - `Read/Write hits and misses`
  - `Main memory read/write counts`
  - `Average memory access time`

---

## Configuration

Edit these parameters in `main()` to change cache behavior:

```c
cache->politica_write = 1;           // 0 = Write-through, 1 = Write-back
cache->politica_subs = 1;            // 0 = Random, 1 = LRU
cache->vias_associacao = 2;          // Number of lines per set (associativity)
cache->tam_linha = 64;               // Line/block size in bytes
cache->tamanho_total_cache = 8192;   // Total cache size in bytes
```

---

## Input File

The simulator reads memory access operations from a file named `oficial.cache`.

Each line in the file should contain:
- A **memory address** in hexadecimal format.
- A single character operation:
  - `R` for **read**
  - `W` for **write**

---

## File Structure

A quick overview of the key files in this project:

| File                    | Description                                      |
|-------------------------|--------------------------------------------------|
| `cache_simulator.cpp`   | Main source code that implements the cache logic |
| `oficial.cache`         | Input trace file containing memory operations    |
| `README.md`             | Project documentation (this file)                |
