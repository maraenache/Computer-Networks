# Computer-Networks Project
  Proiectul ales, Offline Messenger, implică dezvoltarea unei aplicații de tipul client/server, ce permite schimbul de mesaje între utilizatorii conectați.
 
 Funcțonalitatea aplicației constă în conectarea clienților la server și executarea comenzilor de care vor dispune odată cu logarea în aplicație (onlineUsers,
chat \<username>, getInbox, help etc.). Pentru a putea trimite un mesaj, utilizatorul
trebuie să fie logat, dar utilizatorul căruia îi este adresat mesajul poate fi online sau
offline.
Fiecare utilizator ce primește un mesaj poate răspunde mesajului (reply) sau
îl poate ignora. Serverul îi va permite clientului
să răspundă unor anumite mesaje primite (selectate prin identificatori).  Cu ajutorul
comenzii replyto \<id> \<mesaj> se va trimite mesajul răspuns în conversație. Comanda
refresh permite actualizarea conversației.

Aplicația va folosi o arhitectură de tipul client/server TCP concurent. În această
arhitectură serverul are unul sau mai mulți clienți conectați printr-o adresă și un
port.

Mai multe detalii de implementare, informații despre funcționalitate, arhitectura, scenarii de utilizare sunt prezentate în documentația atașată.

Limbajul ales pentru realizarea proiectului este C.

Baza de date este construită cu SQLITE.
