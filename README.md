# ESIOS
Display precio horario ESIOS 
![20230627_112923](https://github.com/cetrespeo/ESIOS/assets/14995096/ad554e90-e4c2-4e3c-bcb8-0f8cfedf11d3)

Aplicacion Arduino para mostrar los precios de la energia electrica tanto con la curva ESIOS como OMIE en un LilyGo T5.

HARDWARE: 
- Placa: Es compatible con las opciones de Hardware LilyGo T5 tanto de 2,13" como de 2,66". Se elige entre una u otra simplemente comentando el primer #define.
- Bateria:
- Carcasa PLA/PETG: La carcasa esta aun por afinar ...

SOFTWARE:
Para compilar, tener en cuenta las version de las librerias utilizadas segun se indica en el primer comentario.

OPERACION:

Para cargar los datos del WIFI en el equipo:
  1. Reiniciar con el primer boton de la T5
  2. en menos de un segundo, pulsar (y mantener pulsado) el segundo boton hasta que salga un mensaje y seguir las instrucciones
    a. Conectarse al SSID 'ESP32' (quitando la conexion de datos de movil si lo hacemos con el movil)
    b. ir a la pagina 192.168.4.1 con un navegador, rellenar tanto SSID como PASSWORD y salvar.
  3. Tambien se puede hardcodear en el .ino con los primeros defines.
 
Se carga la curva en cuanto se detecta que no se tiene la curva del dia actualizada (sobre las 0h00). A partir de entonces, el display solo actualiza cada hora el valor segun la curva cargada, minimizando el consumo.

Para cambiar de modos ESIOS a OMIE (o de Vertical a Horizontal), hay que reiniciar el equipo con el primer boton de la T5, y en menos de 1 segundo pulsar y mantener pulsado durante 4 segundos el segundo boton del equipo. Despues de soltar el boton, incrementara el modo haciendo un ciclo (ESIOS vertical -> OMIE vertical -> ESIOS horizontal -> OMIE horizontal) 



