# PanBeeMon

* Monitorización de colmenas en remoto\
  Se mediran 7 parametros con sus correspondientes MQTT topics:
   * Temperatura interior\
      `/apiary{id}/hive{ID}/inTemp`
   * Humedad interior\
   `/apiary{id}/hive{ID}/inHum`
   * Inclinacion pitch\
   `/apiary{id}/hive{ID}/pitch`
   * Inclinacion yaw\
   `/apiary{id}/hive{ID}/yaw`
   * Inclinacion roll\
   `/apiary{id}/hive{ID}/roll`
   * Temperatura exterior\
   `/apiary{id}/hive{ID}/exTemp`
   * Peso\
   `/apiary{id}/hive{ID}/weight`

   
  


## Diagrama General
* **First Option**
```mermaid
flowchart LR
    colmenas["` **COLMENA**
    (esp8266)
    ***Sensor temp/hum***
    (dht11)
    ***Acelerometro***
     (MPU6050)
    ***Celdas Carga***
    (Sen10245)`"]
    concentrador["`**CONCENTRADOR**
    (LilyGo Sim7000E)
    ***Sensor Temperatura Exterior***
    (DHT11)`"]
    receptor["` **TAGIo**`"]
    colmenas -- esp_now ---> concentrador -- mqtt ---> receptor
    
```
* Second Option
```mermaid
flowchart LR
    colmenas["` **COLMENA**
    (esp32 C3)
    ***Sensor temp/hum***
    (dht11)
    ***Acelerometro***
     (MPU6050)
    ***Celdas Carga***
    (Sen10245)`"]
    concentrador["`**CONCENTRADOR**
    (LilyGo Sim7000E)
    ***Sensor Temperatura Exterior***
    (DHT11)`"]
    receptor["` **TAGIo**`"]
    colmenas -- esp_now ---> concentrador -- mqtt ---> receptor
    
```

## Diagramas y conexiones entre componentes
### Celdas de carga
* **UL** Up-Left
```mermaid
flowchart TB;
  UL-- Red --- DL
  UL-- Green E-  --- Hx711
  UL-- White --- UR 
```

* **UR** Up-Right
```mermaid
flowchart TB;
  UR-- Red --- LR
  UR-- Green A-  --- Hx711
  UR-- White --- UL 
```
* **DL** Down-Left
```mermaid
flowchart TB;
  DR -- White --- DL 
  Hx711-- Green A+ ---  DL
  UL-- Red ---  DL
```

* **DR**Down-Right
```mermaid
flowchart TB;
  DL-- White ---  DR
  Hx711-- Green E+ ---  DR
  UR-- Red ---  DR
```

* **Diagram**
![Alt text](./images/image-1.png)

* **Giroscopio**
  ![alt text](image.png)

