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
app.use('/map', express.static(path.join(__dirname, "public")));


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

    const { startLocation, endLocation } = req.body;

    if (!startLocation || !endLocation) {
      return res.status(400).send({ error: 'Start and end required' });
    }

    console.log('Received request to calculate path from:', startLocation, 'to:', endLocation);

    const request = JSON.stringify({ startLocation, endLocation });

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