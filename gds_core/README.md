# gds_core

`gds_core` is a C++ 20 library for reading GDSII Stream Format (`.gds`) files and building an internal object model of the library, structures, and elements.
(This implementation follows the GDSII format description and BNF specification published by [Klaas Holwerda](https://boolean.klaasholwerda.nl/interface/bnf/gdsformat.html)).

The project is designed for:
- reliable reading of binary GDS files;
- parsing records according to the GDSII structure;
- building the `Library -> Structure -> Element` model;
- further use of the extracted geometry in GUI applications and analysis algorithms.

### Implemented Features

The library currently supports:

- reading a GDSII stream file from a file or from `std::istream`;
- parsing binary records with proper handling of:
  - `length`
  - `record type`
  - `data type`
- decoding the main GDS data types:
  - `NoData`
  - `BitArray`
  - `Int2`
  - `Int4`
  - `Real8`
  - `ASCII`
- building the object model:
  - `GdsLibrary`
  - `GdsStructure`
  - `GdsElement`
- supporting the main GDS element types:
  - `BOUNDARY`
  - `PATH`
  - `SREF`
  - `AREF`
  - `TEXT`
  - `NODE`
  - `BOX`
- reading the main library-level records:
  - `HEADER`
  - `BGNLIB`
  - `LIBNAME`
  - `REFLIBS`
  - `FONTS`
  - `ATTRTABLE`
  - `GENERATIONS`
  - `FORMAT`
  - `MASK`
  - `UNITS`
  - `ENDLIB`
- reading structure-level records:
  - `BGNSTR`
  - `STRNAME`
  - `STRCLASS`
  - `ENDSTR`
- reading element-level records:
  - `LAYER`
  - `DATATYPE`
  - `TEXTTYPE`
  - `NODETYPE`
  - `BOXTYPE`
  - `PATHTYPE`
  - `WIDTH`
  - `XY`
  - `SNAME`
  - `COLROW`
  - `STRING`
  - `PRESENTATION`
  - `STRANS`
  - `MAG`
  - `ANGLE`
  - `ELFLAGS`
  - `PLEX`
  - `PROPATTR`
  - `PROPVALUE`
  - `ENDEL`

### Implementation Details

##### 1. Layered Design
The code is divided into three layers:

- **low-level input reading**
  - `InputStream`
  - reading bytes and big-endian values

- **raw GDS record reading**
  - `Record`
  - `RecordReader`

- **syntactic GDS parsing**
  - `Parser`
  - construction of `GdsLibrary / GdsStructure / GdsElement`

This design makes it possible to test separately:
- binary input reading;
- record reading;
- GDS structure parsing;
- element validation.

##### 2. Geometry Validation
Basic validation is performed for elements:
- for `BOUNDARY`, the first and last points must match;
- for `BOX`, exactly 5 points are required;
- for `AREF`, both `COLROW` and 3 coordinates must be present;
- for `SREF` and `TEXT`, a single point is expected;
- required fields are checked for the presence of the corresponding records.

##### 3. Support for Zero Padding After `ENDLIB`
The library supports files that contain a tail of zero bytes after `ENDLIB`.

This is important because some GDS files are padded with zeros to align with a physical block size.  
If only `0x00` bytes follow `ENDLIB`, such trailing data is treated as valid.  
If any non-zero data appears after `ENDLIB`, it is treated as an error.

<details>
<summary><strong>Project Structure</strong></summary>

```text
gds_core/
├─ CMakeLists.txt
├─ include/
│  ├─ GdsReader.hpp
│  ├─ GdsLibrary.hpp
│  ├─ GdsStructure.hpp
│  ├─ GdsElements.hpp
│  └─ GdsTypes.hpp
├─ src/
│  ├─ GdsReader.cpp
│  ├─ GdsLibrary.cpp
│  ├─ GdsStructure.cpp
│  ├─ GdsElements.cpp
│  └─ internal/
│     ├─ InputStream.hpp
│     ├─ InputStream.cpp
│     ├─ Record.hpp
│     ├─ Record.cpp
│     ├─ RecordReader.hpp
│     ├─ RecordReader.cpp
│     ├─ Parser.hpp
│     └─ Parser.cpp
└─ tests/
   ├─ CMakeLists.txt
   └─ ReaderTests.cpp
```

</details>


