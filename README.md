# 3-Semester-VERTS-TW-Mailer-Basic

#Beschreibung
1.Der Client wird mit einer IP Adresse und einem Port als Parameter gestartet

2.Der Server wird mit einem Port und einem Verzeichnispfad (Mailspoolverzeichnis) als Parameter gestartet und soll als iterativer Server ausgelegt werden (keine gleichzeitigen Requests)

3.Der Client connected mittels Stream Sockets über die angegebene IP-Adresse / Port Kombination zum Server und schickt Requests an den Server.

4.Der Server erkennt und reagiert auf folgende Requests des Clients: SEND: Senden einer Nachricht vom Client zum Server.


#Befehle
• SEND: client sends a message to the server. 

• LIST: lists all messages of a specific user.

• READ: display a specific message of a specific user.

• DEL: removes a specific message.

• HELP: help

• QUIT: logout the client.