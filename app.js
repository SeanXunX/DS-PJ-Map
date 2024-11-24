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


const wss = new WebSocket.Server({ port: 3001 });

app.use(express.json());

app.post('/calculate-path', (req, res) => {
  try {

    const { startLocation, endLocation } = req.body;

    if (!startLocation || !endLocation) {
      return res.status(400).send({ error: 'Start and end required' });
    }

    console.log('Received request to calculate path from:', startLocation, 'to:', endLocation);


    const processFile = './src/process';
    const args = [startLocation, endLocation];

    execFile(processFile, args, (error) => {
      if (error) {
        console.error('Error executing process program:');
        return res.status(500).send({ error: 'Failed to calculate path.' });
      }

      const shortestGeoJson = './public/shortest_path.geojson';
      if (fs.existsSync(shortestGeoJson)) {
        const geojson = fs.readFileSync(shortestGeoJson, 'utf-8');
        wss.clients.forEach((client) => {
          if (client.readyState == WebSocket.OPEN) {
            client.send(geojson);
          }
        });
      } else {
        return res.status(500).send({ error: 'GeoJSON result not found.' });
      }

      // res.send({ message: 'Path calculated and updated successfully.' });

    });
  } catch (err) {
    console.error('Server error:', err);
    res.status(500).send({ error: 'An unexpected error occurred.' });
  }
});