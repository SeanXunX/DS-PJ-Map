document.addEventListener('DOMContentLoaded', () => {

    var map = L.map('map').setView([31.300917, 121.497785], 15);

    L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
        maxZoom: 19,
        attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
    }).addTo(map);

    const fetchSearchResults = async (query) => {
        try {
          const response = await fetch('/fuzzy-search', {
            method: 'POST',
            headers: {
              'Content-Type': 'application/json',
            },
            body: JSON.stringify({ locationName: query }),
          });
      
          if (!response.ok) {
            throw new Error('Failed to fetch search results');
          }
      
          const results = await response.json();
          return results;
        } catch (error) {
          console.error('Error fetching search results:', error);
          return [];
        }
      };
      
      const debounce = (callback, delay) => {
        let debounceTimer;
        return (...args) => {
          clearTimeout(debounceTimer);
          debounceTimer = setTimeout(() => callback(...args), delay);
        };
      };
      
      // 渲染函数：接受结果、结果容器和目标输入框
      const renderSearchResults = (results, resultsContainer, inputElement) => {
        resultsContainer.innerHTML = '';
      
        if (results.length === 0) {
          resultsContainer.innerHTML = `<div class="no-results">No results found</div>`;
          return;
        }
      
        results.forEach((result) => {
          const item = document.createElement('div');
          item.className = 'search-result-item';
          item.textContent = result;
          item.addEventListener('click', () => {
            inputElement.value = result;
            resultsContainer.innerHTML = '';
          });
          resultsContainer.appendChild(item);
        });
      };
      
      // process input event
      const handleInput = debounce(async (event, resultsContainer, inputElement) => {
        const query = event.target.value.trim();
      
        if (query === '') {
          resultsContainer.innerHTML = '';
          return;
        }
      
        const results = await fetchSearchResults(query);
        renderSearchResults(results, resultsContainer, inputElement);
      }, 300);
      
 
      const startInput = document.getElementById('start-input');
      const startResults = document.getElementById('start-results');
      startInput.addEventListener('input', (event) =>
        handleInput(event, startResults, startInput)
      );
      
      const endInput = document.getElementById('end-input');
      const endResults = document.getElementById('end-results');
      endInput.addEventListener('input', (event) =>
        handleInput(event, endResults, endInput)
      );
      


    let routeLayer = null;

    function updateMap(geojson) {

        if (routeLayer) {
            map.removeLayer(routeLayer);
        }

        routeLayer = L.geoJSON(geojson, {
            style: function (feature) {
                return {
                    color: "#6688ff",
                    weight: 10
                };
            }
        }).addTo(map);
        map.fitBounds(routeLayer.getBounds());
    }

    const navigateBtn = document.getElementById("nav-btn");
    // Function to calculate and display the shortest path
    navigateBtn.addEventListener('click', async () => {
        const startName = startInput.value.trim();
        const endName = endInput.value.trim();

        if (!startName || !endName) {
            alert('Please enter both start and end locations!');
            return;
        }

        try {
            const response = await fetch('/calculate-path', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ startLocation: startName, endLocation: endName }),
            });

            const result = await response.json();
            if (response.ok) {
                // alert(result.message || 'Path calculation started.');
                // console.log('Calculate result:', result);
                updateMap(result);
            } else {
                alert(`Error: ${result.error || 'Failed to calculate path.'}`);
            }

        } catch (error) {
            console.error('Error:', error);
            alert('An error occurred while sending the request.');
        }
    });

    function onMapClick(e) {
        popup
            .setLatLng(e.latlng)
            .setContent("You clicked the map at " + e.latlng.toString())
            .openOn(map);
    }

    map.on('click', onMapClick);

});    
