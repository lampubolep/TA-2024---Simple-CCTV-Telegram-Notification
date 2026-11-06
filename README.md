# TA-2024---Simple-CCTV-Telegram-Notification
Simple CCTV and Telegram Notification with Motion Detector using ESP32 Cam and PIR sensor

Proyek tugas akhir jurusan Rekayasa Elektro 2024.
Saat itu banyak tugas akhir yang sama, ya mirip-mirip lah. Bedanya, untuk sistem onlinenya saya menggunakan SSH Reverse Tunneling ke VPS. Alternatif mudahnya bisa pakai ngrok, tapi untuk versi gratisnya alamat yang digunakan berubah-ubah. Untuk alamat URL statis perlu versi berbayar. Kalo sama-sama berbayar sekalian aja beli VPS sama domain murah (.my.id), di reverse ke sana biar bisa diakses secara online.

Belakangan saya baru tahu kalo cara seperti ini tidak secure, karena asal tahu IP addressnya, CCTV ini bisa diakses siapapun :D Bahkan menggunakan platform seperti Shodan.io, kita bisa mengecek banyak IP yang digunakan untuk CCTV. Selain itu, penggunaan sensor PIR ini saya rasa kurang pas karena sering false notification. Sensor tidak bisa membedakan orang, hewan, pohon yang kena angin, motor lewat :D Perlu pengembangan lebih jauh biar jadi lebih sempurna. Kalo males ya pake aja CCTV Cina, murah-murah tapi nanti datanya diarahin ke server mereka huehehe.

Btw buat yang mau nyoba :
ssh -R 8080:192.168.1.184/stream namauser@IP-VPS

-R untuk reverse SSH | 8080 port VPS | 192.168.1.184/stream - alamat IP lokal ESP32 cam

<img width="637" height="507" alt="Rangkaian" src="https://github.com/user-attachments/assets/d855e2bd-9de1-4a0d-ac90-611572a87275" />

<img width="1007" height="670" alt="Diagram-Blok" src="https://github.com/user-attachments/assets/0d9f8c91-f5d7-4c38-bc1c-af95c06d62c8" />
