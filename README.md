Smart Home Display-Komponente (ESP32)

Dieses Repository enthält die Software für die Display-Komponente eines Smart-Home-Systems. Die Aufgabe dieses Moduls ist es, Sensordaten zu empfangen, zu verarbeiten und übersichtlich auf einem TFT-Display darzustellen. Die Daten werden von externen Sensor-Komponenten über das MQTT-Protokoll übertragen.

Als zentrale Steuereinheit wird ein ESP32 eingesetzt. Er übernimmt sowohl die WLAN- und MQTT-Kommunikation als auch die Ansteuerung des Displays über SPI und des RTC-Moduls über I2C. Zusätzlich zeigt die Display-Komponente Datum und Uhrzeit an, welche über ein RTC-Modul bereitgestellt und regelmäßig über NTP synchronisiert werden.

Funktionsweise

Nach dem Einschalten initialisiert der ESP32 die angeschlossenen Komponenten und stellt automatisch eine Verbindung zum WLAN her. Anschließend wird eine verschlüsselte MQTT-Verbindung zu einem Cloud-Broker aufgebaut. Sobald diese Verbindung besteht, abonniert das System die entsprechenden Topics und beginnt mit dem Empfang der Sensordaten.

Die empfangenen Daten liegen im JSON-Format vor und enthalten Messwerte für Temperatur, Luftfeuchtigkeit und Luftdruck. Diese Werte werden verarbeitet und in zwei Spalten auf dem Display dargestellt, sodass die Daten von zwei Sensor-Komponenten gleichzeitig sichtbar sind. Parallel dazu werden Datum und Uhrzeit im oberen Bereich des Displays angezeigt.

Der aktuelle Systemzustand ist jederzeit direkt am Display erkennbar. Sowohl der WLAN-Status als auch der MQTT-Status werden visuell dargestellt, wodurch Verbindungsprobleme schnell erkannt werden können. Zusätzlich gibt der ESP32 Status- und Debug-Informationen über die serielle Schnittstelle aus.

Technik im Überblick

Hardwareseitig besteht die Display-Komponente aus einem ESP32-Entwicklungsboard, einem TFT-Display mit ILI9341-Controller sowie einem RTC-Modul vom Typ DS3231. Die Stromversorgung erfolgt über einen 3,7-V-Akku, der über ein Lade- und ein Step-Up-Modul betrieben wird. Ein Ein-/Ausschalter ermöglicht den manuellen Betrieb.

Softwareseitig kommt das Arduino-Framework für ESP32 (Plattform espressif32) zum Einsatz. Dieses stellt alle notwendigen Funktionen für WLAN, sichere TLS-Verbindungen, SPI- und I2C-Kommunikation sowie Zeitfunktionen bereit. Ergänzt wird das Framework durch gängige Arduino-Bibliotheken für Display-Ansteuerung, MQTT-Kommunikation und JSON-Verarbeitung.

Der vollständige Verschaltungsplan ist im Repository separat abgelegt.

MQTT und Zeitmanagement

Die Display-Komponente empfängt Sensordaten über MQTT in Form von JSON-Nachrichten. Diese werden direkt nach dem Empfang verarbeitet und in der Anzeige aktualisiert. Die Uhrzeit wird primär über das RTC-Modul bereitgestellt. Nach erfolgreichem WLAN-Verbindungsaufbau erfolgt eine automatische Synchronisation über einen NTP-Server, um eine dauerhaft korrekte Zeit sicherzustellen.

Inbetriebnahme

Nach dem Aufbau der Hardware gemäß Verschaltungsplan kann die Firmware auf den ESP32 geflasht werden. Nach dem Einschalten verbindet sich das System selbstständig mit dem WLAN und dem MQTT-Broker. Sobald Sensordaten empfangen werden, erscheinen diese automatisch zusammen mit Datum und Uhrzeit auf dem Display.

Hinweise und Ausblick

Die Display-Komponente ist für einen dauerhaften Betrieb ausgelegt. Aktuell wird kein softwareseitiger Energiesparmodus verwendet; der Betrieb erfolgt über einen manuellen Ein-/Ausschalter. Zukünftige Erweiterungen könnten unter anderem Energiesparfunktionen, eine flexiblere Konfiguration der Netzwerkdaten oder die Unterstützung zusätzlicher Sensor-Module umfassen.

Autor

Timo Locker
Display-Komponente (SPI & RTC)
