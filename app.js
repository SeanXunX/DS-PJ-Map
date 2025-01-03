const express = require('express');
const bodyParser = require('body-parser');
const WebSocket = require('ws');
const { execFile, exec } = require('child_process');
const fs = require('fs');
const app = express();
const port = 3000;

app.get('/', (req, res) => {
  res.send('Hello World!')
});

app.post('/', (req, res) => {
  res.send('Got a POST request')
});

app.put('/user', (req, res) => {
  res.send('Got a PUT request at /user')
});

app.delete('/user', (req, res) => {
  res.send('Got a DELETE request at/user')
});

app.listen(port, () => {
  console.log(`Example app listening on port ${port}`)
});

const path = require('path');
const { error } = require('console');
app.use('/map', express.static(path.join(__dirname, "public")));

app.use('/rendering', express.static(path.join(__dirname, "render")));

const cppSocket = new WebSocket('ws://localhost:3002');

// WebSocket 消息处理
cppSocket.on('open', () => {
  console.log('Connected to C++ WebSocket server');
});

cppSocket.on('error', (err) => {
  console.error('WebSocket error:', err);
});

cppSocket.on('message', (message) => {
  console.log('Received from C++:', message);
});

app.use(express.json());

app.post('/calculate-path', (req, res) => {
  try {

    const { startLocation, endLocation, type } = req.body;

    if (!startLocation || !endLocation) {
      return res.status(400).send({ error: 'Start and end required' });
    }

    console.log('Received request to calculate path from:', startLocation, 'to:', endLocation, 'type:', type);

    let request = JSON.stringify();
    if (type == "car") {
      request = JSON.stringify({ queryType: 'path', startLocation, endLocation });
    } else if (type == "ped") {
      request = JSON.stringify({ queryType: 'ped_path', startLocation, endLocation });
    }

    cppSocket.send(request, (err) => {
      if (err) {
        console.error('Error sending to C++ server:', err);
        return res.status(500).send({error: 'C++ server error'})
      }

      cppSocket.once('message', (message) => {
        try {
          const result = JSON.parse(message);
          res.send(result);
        } catch (error) {
          console.error('Error parsing c++ response:', error);
          res.status(500).send({error: 'c++ return type error'});
        }
      });
    });

  } catch (err) {
    console.error('Server error:', err);
    res.status(500).send({ error: 'An unexpected error occurred.' });
  }
});

app.post('/fuzzy-search', (req, res) => {
  try {
    const { locationName } = req.body;

    if (!locationName) {
      return res.status(400).send({ error: 'Location name required' });
    }

    console.log('Received fuzzy search request for:', locationName);

    // 准备 WebSocket 请求
    const request = JSON.stringify({ queryType: 'fuzzy', locationName });

    cppSocket.send(request, (err) => {
      if (err) {
        console.error('Error sending to C++ server:', err);
        return res.status(500).send({ error: 'C++ server error' });
      }

      cppSocket.once('message', (message) => {
        try {
          const results = JSON.parse(message);
          res.send(results);
        } catch (error) {
          console.error('Error parsing C++ response:', error);
          res.status(500).send({ error: 'C++ response parse error' });
        }
      });
    });
  } catch (err) {
    console.error('Server error:', err);
    res.status(500).send({ error: 'An unexpected error occurred.' });
  }
});

app.post('/calculate-route-arbitrary', (req, res) => {
  try {
    const { start, end } = req.body;

    if (!start || !end || start.length !== 2 || end.length !== 2) {
      return res.status(400).send({ error: 'Start and end coordinates are required and must be valid' });
    }

    console.log('Received request to calculate path from:', start, 'to:', end);

    const request = JSON.stringify({
      queryType: 'arbitrary',
      startLocation: { lat: start[0], lng: start[1] },
      endLocation: { lat: end[0], lng: end[1] }
    });

    cppSocket.send(request, (err) => {
      if (err) {
        console.error('Error sending to C++ server:', err);
        return res.status(500).send({error: 'C++ server error'})
      }

      cppSocket.once('message', (message) => {
        try {
          const result = JSON.parse(message);
          res.send(result);
        } catch (error) {
          console.error('Error parsing c++ response:', error);
          res.status(500).send({error: 'c++ return type error'});
        }
      });
    });

  } catch (err) {
    console.error('Server error:', err);
    res.status(500).send({ error: 'An unexpected error occurred.' });
  }
});
