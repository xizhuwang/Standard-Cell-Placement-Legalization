Before:

![螢幕擷取畫面 2024-10-20 115945](https://github.com/user-attachments/assets/e86c14be-5c36-40e9-b40f-b5056a3e2dbf)

After:

![螢幕擷取畫面 2024-10-31 002007](https://github.com/user-attachments/assets/81cf49c0-386b-4f7a-ac10-cbf86db08eba)


# Standard-Cell-Placement-Legalization

## 1. Programming Information

- **Programming Language**: C++
- **Compilation Environment**: `gcc` version 13.2.0 (Ubuntu 13.2.0-23ubuntu4)
- **Visualization Tool**: `Viewer.py`

## 2. Installation

To install the required compiler:

```sh
sudo apt-get install g++
```

For building the environment:

```sh
sudo apt-get install build-essential
```

Recommended editor:

- **VSCode**

## 3. Compilation & Execution

To compile the program:

```sh
g++ legalizer.cpp -o legalizer -Wall
```

To execute:

```sh
./legalizer ibm05 output02
```

> **Note**: The required files will be automatically retrieved, no need to specify file extensions.

To list output files:

```sh
ls output.*
```

## 4. Program Flow

1. **Read Input Files**  
   - The `wts` and `nets` files are **not used**.  
   - The `aux` file defines the circuit.  
   - The `scl` file defines the row information.  
   - The `pl` file defines the coordinates.  
   - The `nodes` file defines the **length** and **width** of cells.

2. **Initial Legalization**  
   - Place cells into the **nearest row**, following the order:
     - **Left to right**
     - **Top to bottom**
     - **Largest to smallest**  
   - If the **nearest row is full**, move to the next closest row.

3. **Secondary Optimization**  
   - Compute **Manhattan distance** (`x + y`) and sort cells.
   - Swap cells within a given Manhattan range to **minimize displacement**.
   - If no better position is found, the placement remains unchanged.

4. **Iteration & Convergence**  
   - The **maximum** number of optimization iterations is **6**.
   - If convergence is achieved earlier, the process exits.

5. **Final Output**  
   - Compute **total displacement** and **maximum displacement**.
   - Write results to a file (**accuracy: 6 decimal places**).

## 5. Execution Time (Performance Test)

Tested on **Intel(R) i7-1165G7 / 16GB RAM**.

| Benchmark | Execution Time | Total Displacement | Max Displacement |
|-----------|--------------|-------------------|----------------|
| toy       | < 1 sec      | 44005.6000       | 2337.6000      |
| ibm01     | 17 sec      | 36962324.9663     | 35641.2000     |
| ibm05     | 127 sec     | 2224209.8687      | 525.5000       |

---

### 🔹 **Notes**
- The program implements a **basic legalization** algorithm followed by **local optimization**.
- The goal is to minimize **displacement** while keeping cells within the defined rows.
