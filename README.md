# Data Structure Project

## QuickStart

1. Build the c++ program. 

``` shell
mkdir build && cd ./build
cmake .. && make
```

2. Start c++ process server and web server.

```shell
./bin/process
node app.js
```


3. Open `http://localhost:3000/map` at browser.


## Milestone

### Basic functions

#### Data Process

- osmium tool
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp)

#### Shortest Path Algorithm
- [Efficient Point-to-Point Shortest Path Algorithms](https://www.cs.princeton.edu/courses/archive/spr06/cos423/Handouts/EPP%20shortest%20path%20algorithms.pdf)

#### Backend

- [websocketpp](https://github.com/zaphoyd/websocketpp)
- Express: web application framework

#### Frontend
- Leaflet
- Bootstrap

...


### Extra functions
- Shortest way between arbitrary points.
- Different traffic methods.
- Nearest neighbour search with **KD Tree**.
- Improved real life shortest way algorithm (higher priority of expressway and motorway when driving a car). 
- Fuzzy search location name.

  [rapidfuzz](https://github.com/rapidfuzz/rapidfuzz-cpp)

### Todo list 
- [ ] Render my own tile using Mapnik. (Now using leaflet's openstreetmap.)

