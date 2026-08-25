# KIDSENTINEL: Bebek İzleme Sistemi Teknik Tasarım ve Algoritma Analizi

## 1. Giriş ve Proje Vizyonu
KIDSENTINEL projesi, bebeklerin hayati belirtilerini (nabız, vücut sıcaklığı ve ortam nemi/ısısı) kesintisiz bir şekilde takip etmek amacıyla geliştirilmiş, IoT tabanlı gelişmiş bir biyomedikal izleme sistemidir. Sistem, modern mikrodenetleyici mimarileri ile hassas biyosensör teknolojilerini birleştirerek, hastane tipi monitörlerin sunduğu kritik verileri ev ortamına taşır.

## 2. Donanım Mimarisi ve Bileşenler

### 2.1. ESP32-C6: Yeni Nesil Kablosuz Denetleyici
- **İşlemci**: 160 MHz hızında çalışan 32-bit RISC-V çekirdeği.
- **Bellek**: 512 KB SRAM ve 4 MB Flash bellek.
- **Wi-Fi 6 (802.11ax)**: Kalabalık ev içi ağlarda düşük parazit ve kararlı kablosuz iletim.
- **I2C Konfigürasyonu**: GPIO 0 (SDA) ve GPIO 1 (SCL) pinleri, 400kHz stabilite hızında çalışmaktadır.

### 2.2. MAX30100 / MAX30102: Fotopletismografi (PPG) Sensörü
- **Bileşenler**: 660nm (Kırmızı) ve 880nm (Kızılötesi) LED optik fotodedektör.
- **Entegre Filtre**: Ortam ışığından kaynaklanan gürültüleri filtreleyen donanımsal analog ön uç (AFE).

### 2.3. SHT31: Hassas Çevresel Sensör
- **Doğruluk**: $\pm 0.2^\circ\text{C}$ sıcaklık ve $\pm \%2$ bağıl nem hassasiyeti. Bebeğin uyku ortamının "Termal Nötral Bölge" içinde kalmasını sağlar.

## 3. Algoritma Analizi ve Sinyal İşleme

### 3.1. Fotopletismografi (PPG) Teorisi: Beer-Lambert Yasası
PPG sinyali, dokudaki kan hacminin kalbin her atışıyla değişmesi sonucu oluşan ışık emilim varyasyonudur. **Beer-Lambert Yasası**:

$$I = I_0 \cdot e^{-(\mu_a + \mu_s) \cdot d}$$

Burada:
- $\mu_a$: Absorbsiyon (emilim) katsayısı.
- $\mu_s$: Saçılma katsayısı.
- $d$: Dokunun derinliği.

### 3.2. Kalp Atış Hızı Tespit Algoritması
1. **Hareket Gürültüsü Filtreleme**: Bebeklerin hareketlerinden kaynaklanan ani sıçramalar eşikleme ile temizlenir.
2. **Kayan Pencere / EMA Filtresi**: $HR_{yeni} = 0.90 \times HR_{eski} + 0.10 \times HR_{olcum}$ formülü ile mikro dalgalanmalar düzeltilir.
3. **Zirve Algılama (Peak Detection)**: Sinyal türevinin sıfır geçişleri tespit edilerek kalp atış zirveleri ($T_{RR}$) belirlenir.
4. **BPM Hesabı**: $BPM = 60000 / T_{RR}$.

## 4. Kablosuz Veri Protokolü ve Güvenlik
- Veriler HTTP POST metodu ile Display Node üzerindeki `/data` endpoint'ine aktarılır (`temp=36.5&ir=110`).
- WPA2 şifrelemeli yerel Access Point mimarisi ile bebek verileri yerel ağda güvenle izole edilir.

## 5. HMI Görüntüleme ve Alarm Mantığı
- **LovyanGFX Kütüphanesi**: ILI9341 SPI TFT ekran üzerinde akıcı ve titremesiz grafik arayüz sağlar.
- **Otonom Alarm**: Kritik eşiklerde (Örn: Nabız < 50 BPM, Ateş > 38.0°C) GPIO 26 piezo hoparlör üzerinden sesli uyarı üretilir.
