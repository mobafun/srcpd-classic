# Patches for Srcpd 2.1.3

srcpd is a daemon to control different model railway hardware by a device independent protocol called SRCPD.

### SRCP
SRCP steht für "Simple Railroad Command Protocol", eine Entwicklung aus dem Usenet
    (news:de.rec.modelle.bahn) um Modellbahnen mit dem Computer zu
    steuern. Die Betonung liegt auf dem S ;=). Die letzte gepflegte Version ist 0.8.4.

siehe http://srcpd.sourceforge.net/srcp/srcp-084.pdf

### srcpd
Autoren:

srcpd ist in gewissem Sinne die Referenzimplementierung für SRCP. Die letzte Version 2.1.3 wird seit 2014 nicht mehr weiter gepflegt, obwohl einige (bekannte) Fehler enthalten sind.
Projekt-Homepage: http://srcpd.sourceforge.net
Dwonload des ursprünglichen Quellcodes: https://sourceforge.net/projects/srcpd/files/

Der srcpd ist eigentlich ein tolles Werkzeug für kleine Modelleisenbahn-Projekte, die einfach nur ein paar Lokomotiven und Weichen ansteuern wollen.

### Patches
Dieses Projekt enthält einige wichtige Bugfixes, um "erst einmal" einen lauffähigen Daemon auf einem Raspberry Pi in KOmbination mit RocRail zu haben. Insgesamt ist der C Code aber nicht mehr zeitgemäß und schwierig wartbar. Mittelfristig wird es deshalb eine Reimplementierung mit moderneren Mitteln geben.

Die GIT Änderungshistorie ist gepfelgt und dient zur Nachverfolgung der Veränderungen seit 2.1.3