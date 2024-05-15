# Deadlock
Codul din acest proiect este capabil sa detecteze anumite situatii de deadlock folosind mutex-uri.
Algoritmul parseaza din fisierele de test mutex-urile, respectiv lock-urile si unlock-urile, retinem aceste mutex-uri intr-un hashmap si folosim DFS pentru a detecta un ciclu.
Daca un ciclu este gasit, atunci inseamna ca am gasit un deadlock.
Programul a fost rulat pe aproximativ 100 teste.
