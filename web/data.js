import { initializeApp } from "https://www.gstatic.com/firebasejs/11.2.0/firebase-app.js";
import { getDatabase, ref, get } from "https://www.gstatic.com/firebasejs/11.2.0/firebase-database.js";


const firebaseConfig = {
      apiKey: "your_apiKey",
      authDomain: "your_authDomain",
      databaseURL: "your_databaseURL",
      projectId: "your_projectId",
      storageBucket: "your_storageBucket",
      messagingSenderId: "your_messagingSenderId",
      appId: "your_appId",
      measurementId: "your_measurementId"
};


const app = initializeApp(firebaseConfig);
const database = getDatabase(app);
const sensorRef = ref(database, 'sensor/');


let waterTempData = [], tempData = [], humidityData = [], phData = [], tdsData = [], turbidityData = [], waterLevelData = [], timeData = [];


function fetchData() {
    get(sensorRef).then((snapshot) => {
        const data = snapshot.val();
        if (data) {
            const timestamp = new Date().toLocaleTimeString();
            document.getElementById('lastUpdateTime').textContent = timestamp;

            function addDataPoint(array, value) {
                if (array.length >= 10) array.shift();
                array.push(value);
            }

            addDataPoint(waterTempData, data.tempDS18B20);
            addDataPoint(tempData, data.tempDHT);
            addDataPoint(humidityData, data.humidity);
            addDataPoint(phData, data.pH);
            addDataPoint(tdsData, data.tds);
            addDataPoint(turbidityData, data.turbidity);
            addDataPoint(waterLevelData, data.waterLevel);
            addDataPoint(timeData, timestamp);

            localStorage.setItem('waterTempData', JSON.stringify(waterTempData));
            localStorage.setItem('tempData', JSON.stringify(tempData));
            localStorage.setItem('humidityData', JSON.stringify(humidityData));
            localStorage.setItem('phData', JSON.stringify(phData));
            localStorage.setItem('tdsData', JSON.stringify(tdsData));
            localStorage.setItem('turbidityData', JSON.stringify(turbidityData));
            localStorage.setItem('waterLevelData', JSON.stringify(waterLevelData));
            localStorage.setItem('timeData', JSON.stringify(timeData));

            updateChart(waterTempChart, waterTempData, timeData);
            updateChart(tempChart, tempData, timeData);
            updateChart(humidityChart, humidityData, timeData);
            updateChart(phChart, phData, timeData);
            updateChart(tdsChart, tdsData, timeData);
            updateChart(turbidityChart, turbidityData, timeData);
            updateChart(waterLevelChart, waterLevelData, timeData);
        }
    }).catch((error) => console.error("Failed to fetch data:", error));
}


function updateChart(chart, data, labels) {
    chart.data.labels = labels;
    chart.data.datasets[0].data = data;
    chart.update();
}


function createChart(ctx, label, color) {
    return new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: label,
                data: [],
                borderColor: color,
                fill: false,
                tension: 0.5
            }]
        },
        options: {
            responsive: true,
            interaction: { mode: 'nearest', intersect: false },
            plugins: {
                tooltip: {
                    enabled: true,
                    mode: 'nearest',
                    intersect: false,
                    backgroundColor: 'rgba(0,0,0,0.7)',
                    titleColor: '#fff',
                    bodyColor: '#fff'
                }
            },
            animation: { duration: 500, easing: 'easeOutQuad' },
            scales: { x: { title: { display: true, text: 'Time' } }, y: { title: { display: true, text: label } } }
        }
    });
}

const waterTempChart = createChart(document.getElementById('waterTempChart').getContext('2d'), 'Water Temperature (°C)', 'rgb(255, 99, 132)');
const tempChart = createChart(document.getElementById('tempChart').getContext('2d'), 'Temperature (°C)', 'rgb(54, 162, 235)');
const humidityChart = createChart(document.getElementById('humidityChart').getContext('2d'), 'Humidity (%)', 'rgb(75, 192, 192)');
const phChart = createChart(document.getElementById('phChart').getContext('2d'), 'pH', 'rgb(153, 102, 255)');
const tdsChart = createChart(document.getElementById('tdsChart').getContext('2d'), 'TDS', 'rgb(255, 159, 64)');
const turbidityChart = createChart(document.getElementById('turbidityChart').getContext('2d'), 'Turbidity', 'rgb(255, 205, 86)');
const waterLevelChart = createChart(document.getElementById('waterLevelChart').getContext('2d'), 'Water Level (cm)', 'rgb(75, 0, 130)');

// Load data from localStorage
function loadDataFromLocalStorage() {
    [waterTempData, tempData, humidityData, phData, tdsData, turbidityData, waterLevelData, timeData] = [
        'waterTempData', 'tempData', 'humidityData', 'phData', 'tdsData', 'turbidityData', 'waterLevelData', 'timeData'
    ].map(key => JSON.parse(localStorage.getItem(key)) || []);

    updateChart(waterTempChart, waterTempData, timeData);
    updateChart(tempChart, tempData, timeData);
    updateChart(humidityChart, humidityData, timeData);
    updateChart(phChart, phData, timeData);
    updateChart(tdsChart, tdsData, timeData);
    updateChart(turbidityChart, turbidityData, timeData);
    updateChart(waterLevelChart, waterLevelData, timeData);
}



// Smooth scrolling effect with highlight
document.querySelectorAll('#nav-mobile a').forEach(anchor => {
    anchor.addEventListener('click', function(event) {
        event.preventDefault();
        const targetId = this.getAttribute('href').substring(1);
        const targetElement = document.getElementById(targetId);
        
        if (targetElement) {
            window.scrollTo({
                top: targetElement.offsetTop - 20,
                behavior: 'smooth'
            });

            setTimeout(() => highlightChart(targetId), 500);
        }
    });
});

// Fetch data every 15 seconds
setInterval(fetchData, 15000);

// Load stored data on page load
window.onload = loadDataFromLocalStorage;

