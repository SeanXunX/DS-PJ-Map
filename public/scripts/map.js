document.addEventListener('DOMContentLoaded', () => {

    var map = L.map('map').setView([31.300917, 121.497785], 15);
    
    L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
        maxZoom: 19,
        attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
    }).addTo(map);

    const searchInput = document.getElementById('location-search');
    const searchBtn = document.getElementById('search-btn');
    const startInput = document.getElementById('start-point');
    const endInput = document.getElementById('end-point');
    const navigateBtn = document.getElementById('navigate-btn');
    const toggleModeBtn = document.getElementById('toggle-mode-btn');
    const navContainer = document.getElementById('nav-container');
    const searchContainer = document.getElementById('search-container');

    let isSearchMode = true;

    // Function to toggle between modes
    toggleModeBtn.addEventListener('click', () => {
        isSearchMode = !isSearchMode;
        navContainer.style.display = isSearchMode ? 'none' : 'block';
        searchContainer.style.display = isSearchMode ? 'block' : 'none';
        toggleModeBtn.textContent = isSearchMode ? 'Toggle to Navigation Mode' : 'Toggle to Search Mode';
    });

    // Get lng and lat from geojson file
    async function readByName(name) {
        var res = new Array();

        const result = await fetch('http://localhost:3000/static/shanghai.geojson');
        const data = await result.json();
        
        // Find the Point the specified name
        for (const feature of data.features) {
            if (feature.geometry.type == 'Point' && name == feature.properties.name) {
                res.push(feature.geometry.coordinates); 
            }
        } 
        return res;
    }
    
    // Function to search for a location by name
    searchBtn.addEventListener('click', async () => {
        const locationName = searchInput.value;
        if (locationName) {
            const locations = readByName(locationName);
            if ((await locations).length != 0) {

            } else {
                alert('Location not found.');
            }
        }
    });

    // Function to calculate and display the shortest path
    navigateBtn.addEventListener('click', async () => {
        const startName = startInput.value;
        const endName = endInput.value;
        
        const response = await fetch(`/calculate-path?start=${encodeURIComponent(startName)}&end=${encodeURIComponent(endName)}`);
        const pathData = await response.json();

        // Display the route on the map
        if (pathData && pathData.coordinates) {
            const pathCoords = pathData.coordinates.map(coord => [coord[1], coord[0]]);
            L.polyline(pathCoords, { color: 'blue' }).addTo(map);
        } else {
            alert('Route not found.');
        }
    }); 
    
    function onMapClick(e) {
        popup
            .setLatLng(e.latlng)
            .setContent("You clicked the map at " + e.latlng.toString())
            .openOn(map);
    }
    
    map.on('click', onMapClick);

    var myGeoStyle = {
        "color": "#ff7800",
        "weight": 10,
        "opacity": 0.65
    };

    var geojsonMarkerOptions = {
        radius: 8,
        fillColor: "#0088aa",
        color: "#000",
        weight: 1,
        opacity: 1,
        fillOpacity: 0.8
    };

    function onEachFeature(feature, layer) {
        // does this feature have a property named popupContent?
        if (feature.properties && feature.properties.popupContent) {
            layer.bindPopup(feature.properties.popupContent);
        }
    }
    
    var geojsonFeature = {
        "type": "Feature",
        "properties": {
            "name": "Coors Field",
            "amenity": "Baseball Stadium",
            "popupContent": "This is where the Rockies play!"
        },
        "geometry": {
            "type": "Point",
            "coordinates": [121.545711,31.2855743]
        }
    };
    
    L.geoJSON(geojsonFeature, {
        onEachFeature: onEachFeature
    }).addTo(map);

    fetch('http://localhost:3000/static/shortest_path.geojson')
        .then(response => {
            if (!response.ok) throw new Error("Network response was not ok");
            return response.json();
        })
        .then(data => {
            console.log("GeoJSON data loaded:", data);  // 添加日志以检查数据
            
            L.geoJSON(data, {
                pointToLayer: function (feature, latlng) {
                    return L.circleMarker(latlng, geojsonMarkerOptions);
                },
                style: myGeoStyle,
                onEachFeature: onEachFeature
            }).addTo(map);

            L.geoJSON(data, {
                style: function (feature) {
                    return {
                        color: "#7800ff",
                        weight: 3
                    };
                }
            }).addTo(map);
        })
        .catch(error => console.error('Error loading GeoJSON:', error));
    
    
    var popup = L.popup()
        .setLatLng([31.300917, 121.497785])
        .setContent("START")
        .openOn(map);

});    
