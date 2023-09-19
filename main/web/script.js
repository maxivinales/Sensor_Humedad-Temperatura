document.addEventListener('DOMContentLoaded', function () {
    document.addEventListener('DOMContentLoaded', function () {
        const networkList = document.getElementById('network-list');
        const networkForm = document.getElementById('connect-form');
        const networkSelect = document.getElementById('network');
        const selectedNetworkInput = document.getElementById('selected-network'); // Nuevo
        // Realizar la solicitud HTTP para obtener la lista de redes
        fetch('http://192.168.4.1/index.js/PullNets')
            .then(response => response.json())
            .then(data => {
                if (data && Array.isArray(data.Nets)) {
                    const networks = data.Nets;
                    const table = document.createElement('table');
    
                    networks.forEach(network => {
                        const tr = document.createElement('tr');
                        tr.innerHTML = `
                            <tr><td class="network-name">${network.SSID}</td></tr>
                            <tr><td><img src="${getLockerImage(network.authmode)}" width="14px"></td></tr>
                            <tr><td><img class="signal-image" src="${getSignalImage(network.rssi)}" alt="Signal Icon"></td></tr>
                        `;
                        tr.dataset.private = network.authmode !== "0";
                        table.appendChild(tr);
            
                        // Captura los clics en el nombre de la red
                        const networkNameCell = tr.querySelector('.network-name');
                        networkNameCell.addEventListener('click', function () {
                            selectedNetworkInput.value = network.SSID;
                        });
                    });
            
                    networkList.appendChild(table);
                
            } else {
                console.error('La respuesta del servidor no contiene una lista de redes válida.');
            }
        })
        .catch(error => {
            console.error('Error al obtener la lista de redes:', error);
        });
    
    
    });
    
    
    networkForm.addEventListener('submit', function (e) {
        e.preventDefault();
        const selectedOption = networkSelect.options[networkSelect.selectedIndex];
        const password = document.getElementById('password').value;
        const selectedNetwork = {
            name: selectedOption.value,
            private: selectedOption.dataset.private === 'true',
            strength: selectedOption.textContent.match(/\(RSSI: ([-\d]+)\)/)[1]
        };
    
        console.log(`Conectando a la red ${selectedNetwork.name} con contraseña ${password}`);
    });
    
    const requestData = {
        "WiFi": {
        "SSID": selectedNetworkInput,
        "Pass": wifiPass,
        "authmode": wifiAuthmode,
        },
        "MQTT": {
        "User": mqttUser,
        "Pass": mqttPass
        }
    };
    
    function enviarDatos() {
        // Realiza la solicitud POST a la URL https://192.168.4.1/connect
        fetch('https://192.168.4.1/connect', {
        method: 'POST',
        headers: {
        'Content-Type': 'application/json'
        },
        body: JSON.stringify(requestData)
        })
        .then(response => response.json())
        .then(data => {
        // Maneja la respuesta del servidor si es necesario
        console.log('Respuesta del servidor:', data);
    })
    
    .catch(error => {
        console.error('Error al realizar la solicitud POST:', error);
        });
    }
    
    document.addEventListener('DOMContentLoaded', function() {
        const connectButton = document.querySelector('#connect-button');
    
        if (connectButton) {
            connectButton.addEventListener('click', function() {
                if (!selectedNetworkInput || selectedNetworkInput.trim() === '') {
                    // Mostrar un mensaje de error
                    alert('Por favor, seleccione una red antes de conectar.');
                    return; // No se realiza la solicitud POST
                }
    
                enviarDatos(); // Llama a la función enviarDatos al hacer clic en el botón "Conectar"
            });
        }
    });
    
    const off_WM_Mode = {
        "off_WM_mode": 1
    }
    
    function cerrarConexion() {
        // Realiza una solicitud POST a la URL https://192.168.4.1/connect
        fetch('https://192.168.4.1/connect', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            //Apaga la conexión
            body: JSON.stringify(off_WM_Mode)
        })
        .then(response => {
            if (response.ok) {
                console.log('Cierre exitoso.');
            } else {
                console.error('Error en la solicitud de cierre:', response.status);
            }
        })
        .catch(error => {
            console.error('Error al realizar la solicitud de cierre:', error);
        });
    }
    
    function getLockerImage(authmode){
    if(authmode !== "0"){
        //return './img/locker-close.png';
        return 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACcAAAAyBAMAAAAtlt8uAAAABGdBTUEAALGPC/xhBQAAAAFzUkdCAK7OHOkAAAAtUExURUdwTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAK22XqcAAAAOdFJOUwCATg4rcJpj3kLJ8rYbTV5xjQAAAPZJREFUOMtjYMADHFe8e9cjhCqm9Q4MWpHFbr2DghaEGOc+mOA7BbjgbSCvrXJ6BpB6CRNj7nv3LgXEcHv37rUBVJDx3bvnEFbdu3cJUMF7794VQFis7969hQrKvXsDM2ndu6dw1hOY4Lx3r6D2vHu3ACbI8e4dxCYWhOkMbO/eOUCc/u5dAEwQaNMEMIMJSZAd5id6CSqKvXt3UBAKZN69S5QEOxcdBDDoYQo+AAYR7QRf6Br1YQguZmDgwRC8AIyCfWiC4Kg5hyYIjs88NMHXIME+dDMDwEGEJriJgWE2puMvMGPxkcNQFkwUJCfi5mETxJKWBACVHkLwLPagmAAAAABJRU5ErkJggg==';
    } else {
        //return'./img/locker-open.png'
        return 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACUAAAAyBAMAAAApYw8TAAAABGdBTUEAALGPC/xhBQAAAAFzUkdCAK7OHOkAAAAtUExURUdwTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAK22XqcAAAAOdFJOUwCBt0DRJnLnowtcTRY4TTvb+gAAAOlJREFUKM9jYIAAnl1iiycwoABuvXfv3j1agCLk9w4EnhggifW9g4BnCCEOIPe5Ux2QFICLyb17VxbAwJOOpJAbaPwEqNwGqNj0d+8UwAyud+8coGJ5754fgBkC01z37hVcxxMIg/fduwKoGNu7dxNgjASoGCvMEqDrLjDAjYFIM8E0AIHeuwacYqzGxgzoYsiARmILBZGA3ztHsQkMLO/QgQLDPgyxB6CwpVzMBVNMleEQutgjsINRxd4CxeLQxB4AxdiJEMOm98kEBk4/dLf4MEzBcN9jmNYhJPbMeB3JcbQOixgHhpgAANARCBkyAFPgAAAAAElFTkSuQmCC';
    }    
    }
    
    function getSignalImage(rssi) {
        if (rssi >= -60) {          // excelent
            return 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABsAAAAkCAQAAADS8YmmAAAAIGNIUk0AAHomAACAhAAA+gAAAIDoAAB1MAAA6mAAADqYAAAXcJy6UTwAAAACYktHRAD/h4/MvwAAAAlwSFlzAAAOwwAADsMBx2+oZAAAAAd0SU1FB+cIDhApNG7udUYAAAH1SURBVDjL7dPLSxRQFMfxz0yj5phQFOZjQAjCMDOiiaBFYEItIiFwEQRR/4C1cFcu2xUUQRLSJmgTCBIiCj0WYhAqEgkFvShF0h4gOeKIzG3hNKmZj01t/J7F5Z7L7557z/1dNvivRJbJFSlRpticPJM+Gze9smy7pDpJu2yXLyMq7bt3Bj014Ntydcs06ZMSspExJ5ObpfRpUra4WoFGFyVF8NWwl0Z8Ma1IiYQaNXYgGHBDu/S8tNxtKUHGkMsOKl5yjmJJLV7ICFJalUO1LkHwXrOKBX2JKYjGFogTmn0QBF2qIzqdRLcrBkGhakcieyIlmbiUca8988oMOOiqE3jITRPa7ARxp3WYyDViPsa1a1AISrUZd41i+xSBve5nOznjo+ee6DcqLQim3FOVfdUaW36f/ZhhQTDillN22ypumyoNWo0JgiFHl75aoW7BrPsO/OGcqEMemBP0iM+nNmWXgl0qXNfiE5GYSvvVKsWUjDE9ZiV0eios3nOzRHaTw+56a0raD2/ckQQxCQV/s3bUBaM5e82PH50VXfkH7PBYrWmPPDFpmzr14obVm7AC+Vr0Ov/r6oqc0+uKfKsQy3kyL+fJ2GqinCU163ApV3WNnJEWzGj8W+eWp1I+Chb+ibVQa0Bav9r1yah0XOV6RRv8I34CCGOjMOKz0ysAAAAldEVYdGRhdGU6Y3JlYXRlADIwMjMtMDgtMTRUMTY6NDE6NDcrMDA6MDBMzkeQAAAAJXRFWHRkYXRlOm1vZGlmeQAyMDIzLTA4LTE0VDE2OjQxOjQ3KzAwOjAwPZP/LAAAACh0RVh0ZGF0ZTp0aW1lc3RhbXAAMjAyMy0wOC0xNFQxNjo0MTo1MiswMDowMPQU8coAAAAASUVORK5CYII=';
        } else if (rssi >= -70) {   // good
            return 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABsAAAAkCAQAAADS8YmmAAAAIGNIUk0AAHomAACAhAAA+gAAAIDoAAB1MAAA6mAAADqYAAAXcJy6UTwAAAACYktHRAD/h4/MvwAAAAlwSFlzAAAOwwAADsMBx2+oZAAAAAd0SU1FB+cIDhApNG7udUYAAAHdSURBVDjL7dM7axRhFMbx38zs7K7ZFWVVQjBFiDe8RC1EghY2KoJa2dj4MSy0E+zMNxAL05nCImBhYSUaUWRBxEJFMBjYRANKou5md2csnGwubBKsbPIfGDjznIdz3nPmZZP/StDlWygUCQVSibZEsr4tFIuRIJUKBAjR1FxuXrJFCgItLe2Olna0WCTVWNSC7F0QWdDMUmJtxFnc1MzivERd+tcWKWpaQF5eItGSZJUCoZxQ0MmI1bUDsVhDW05BYqHT4koieaGGlkhBM1DyW6IoUs8svfY7pF/ZvC/e+mAmsxa11YXKlOUUFbKWjrmt6oe083z32i1DWdWCLXK2klcSgx1umsyS676ZMquRxZ/dUAGxknhpAQfccVGo7qXH3pjyS1m/IRecUJAYd93HlUcuGJNKvXLV9lXjqLimKpUaU1wp5d0z576BLC7pM6hPTxbv9cC8u/Kr/5JeA96Zw25XXNavx0+Txj1UwzYHfcpm2oUzJiTLJpl46tRGN6DikWG8N2Fan2H78MIls9ahZFTNiEEhInuMmDaqZAMqDmd7XBzWkWxj6zS5SM55w555orVRneWc81VqxtnucriG7aSd2OXov9km1FBT7S6vdbbIacdVPV/j/m3yf/kDPt+TPB8V1ekAAAAldEVYdGRhdGU6Y3JlYXRlADIwMjMtMDgtMTRUMTY6NDE6NDYrMDA6MDDquUwkAAAAJXRFWHRkYXRlOm1vZGlmeQAyMDIzLTA4LTE0VDE2OjQxOjQ2KzAwOjAwm+T0mAAAACh0RVh0ZGF0ZTp0aW1lc3RhbXAAMjAyMy0wOC0xNFQxNjo0MTo1MiswMDowMPQU8coAAAAASUVORK5CYII=';
        } else if (rssi >= -80) {   // middle
            return 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABsAAAAkCAQAAADS8YmmAAAAIGNIUk0AAHomAACAhAAA+gAAAIDoAAB1MAAA6mAAADqYAAAXcJy6UTwAAAACYktHRAD/h4/MvwAAAAlwSFlzAAAOwwAADsMBx2+oZAAAAAd0SU1FB+cIDhApNG7udUYAAAGvSURBVDjL7dMxbtRAFMbx39hjezebICFRIaFwAaBBoicFJ0BCSkOOQEGZi1BwB3qUBsqUdIgiUioqEBFZe20PxQ7JEhJIGmjyNeN5nv9738yb4Vr/VeGcSCkqJXNTDHqDdDEWRLVglAxGg1KhFJRGnf4UPsVqlaTLmQsFGI3ZQS1Y6Fax0sSoNQoqFRlagiwsJIVGYW5YYpVKp1doRL1+1Y4giqJeaxQ1OougEbSSRsxZl4sLQTKezCuVXiuYGIN1R4KpUSshqkS9CHpRrzMgaBSOJevBul6ps8h7DDq98cRkIaqNWgMqtUEVRFXO1ih1+lxxaXLQ50ij1+Z/3WkDJpijUeXOJUEpKCy0giavWFFphtKaycrh/7Q5tabATPlru4OZkbyD6I5NG746cJgjjST4fvaalZpc55HXPjnS+uajVx7mms1vPlYM7TiUpNyzJDmwfRY4+wJu2XPPsbf2fHHTY1umPtjy+U8PqbbrnedmeT6z471dtb8o2shfVR5v5BtzCc289MaLk6qX1DOtZO7pRSd3vjbVaNy+WrUH9rX23b8axl1PbF4VutY/0g9e9YvbfiwvtgAAACV0RVh0ZGF0ZTpjcmVhdGUAMjAyMy0wOC0xNFQxNjo0MTo0NyswMDowMEzOR5AAAAAldEVYdGRhdGU6bW9kaWZ5ADIwMjMtMDgtMTRUMTY6NDE6NDcrMDA6MDA9k/8sAAAAKHRFWHRkYXRlOnRpbWVzdGFtcAAyMDIzLTA4LTE0VDE2OjQxOjUyKzAwOjAw9BTxygAAAABJRU5ErkJggg==';
        } else{                     // low
            return 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABsAAAAkCAQAAADS8YmmAAAAIGNIUk0AAHomAACAhAAA+gAAAIDoAAB1MAAA6mAAADqYAAAXcJy6UTwAAAACYktHRAD/h4/MvwAAAAlwSFlzAAAOwwAADsMBx2+oZAAAAAd0SU1FB+cIDhApNG7udUYAAAGSSURBVDjL7dNNctQwEAXgT7Jlzw8XgE3gAnAMTkAVx2DBMbPjIOzC2JYlFlYmQypDJRvY5O2k1ut+3f3EK/4rwhM3nV6nOtljla3qdVrQGwRFtSpWnagTRNUsP5AfaIOkmlvmKIKiNAWDYDE3WlDp7BSTIkgSjbQRWSyqaBSdrFu1JJll0aiX5Us5gl6vl02K3mi2hDAKdVKN+pZ1exwFVTmfkySbBDulC/sPdz+DA1t5vdFeNEqSaK9TFausN8gW++CNrDNbbD0Gs6ycRUa9QTFZkQxWKeglsxWjziyjl5rIVW43o2xqsflhATucMEptc1VoW1tMgrG9uEDniM7B7mL49zL3DiKOuj/XHRwVWgdR0guqbFGayCq42yYbLnImiyIYdVZFEUWd1aSe41esfTScLR2a7Q6PLd8/oiXMOjtEq+Bkbj76y8eJ9oLSFqH9CX5dk3fZYzhX3mz1eLb3A71AVXD0zXdv/TCfffkMfDWpTr5ck/Q0bgwYvXtunQ2f3Jrc+vgyGu99dvNS0iv+EX4DAIaMIv2h9RwAAAAldEVYdGRhdGU6Y3JlYXRlADIwMjMtMDgtMTRUMTY6NDE6NDYrMDA6MDDquUwkAAAAJXRFWHRkYXRlOm1vZGlmeQAyMDIzLTA4LTE0VDE2OjQxOjQ2KzAwOjAwm+T0mAAAACh0RVh0ZGF0ZTp0aW1lc3RhbXAAMjAyMy0wOC0xNFQxNjo0MTo1MiswMDowMPQU8coAAAAASUVORK5CYII=';
        }
    }