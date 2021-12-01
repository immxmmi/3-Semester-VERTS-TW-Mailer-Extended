# 3-Semester-VERTS-TW-Mailer-Extended

#Beschreibung
1.Der Client wird mit einer IP Adresse und einem Port als Parameter gestartet

2.Der Server wird mit einem Port und einem Verzeichnispfad (Mailspoolverzeichnis) als Parameter gestartet und soll als iterativer Server ausgelegt werden (keine gleichzeitigen Requests)

3.Der Client connected mittels Stream Sockets über die angegebene IP-Adresse / Port Kombination zum Server und schickt Requests an den Server.

4.Der Server erkennt und reagiert auf folgende Requests des Clients: SEND: Senden einer Nachricht vom Client zum Server.


#Befehle

• LOGIN: LDAP - LOGIN. (3 Versuche --> Blacklist für 60 Sekunden)

• SEND: Client sendet eine Nachticht an dem Server.

• LIST: listet mithilfe <username> den Betreff von allen Nachrichten

• READ: gibt mithilfe <username> und <number> die MSG aus.

• DELETE: entfernt mithilfe <username> und <number> die MSG.

• HELP: Hilfe -Befehl

• QUIT: Abmelden des Clients.

#Starten

-- Schritt 1 --

Starten Sie zwei Konsolen 

- kompilieren des Programms "make all"
    - Konsole 1 (Server) --> Server Starten --> "./bin/server <ip> <port>"
    - Konsole 1 (Client) --> Client Starten --> "./bin/client <port> <mail-spool-name>"

-- Schritt 2 --

- Befehle ausführen 
  - login, send, list, read, quit, help
  - help erklärt die Befehle.

