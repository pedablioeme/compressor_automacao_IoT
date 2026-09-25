<div align="center">

# Acionamento Remoto de um Compressor de Ar

</div>

Projeto de automação do compressor de ar do IFSC Campus Araranguá por meio do acionamento remoto via WiFi baseado na comunicação ScadaBR-NodeRED-ESP32. O presente código corresponde ao algoritmo implementado no ESP32. Se refere, portanto: <br/>
* à recepção dos comando vindos do nodeRED 
(desejoON,
desejoOFF);

* ao acionamendo do motor responsável pela abertura e fechamento da válvula principal que distribui o ar comprimido ao secador e então à rede, norteados por uma máquina de estados usando a função switch/case; 

* à leitura do status atual do compressor 
(statusEnergizado,
statusLigado,
statusAlivio,
statusSobrecarga,
statusON,
statusOFF,
statusONSecador,
statusOFFSecador);

* ao envio dos estados lidos ao NodeRED usando JSON com protocolo MQTT.

O armazenamento dos dados e hospedagem do servidor é feito por meio de um OrangePI rodando o NodeRED e o ScadaBR ligado à rede do Campus. Neste também, realiza-se a comunicação NodeRED-ScadaBR.


**Ideias Futuras:** <br/>
Uso do servidor para armazenar dados de uso do equipamento (como tempo de uso, temperatura, pressão), propiciando indicadores úteis para manutenção e identificação do atual estado da máquina.

# Funcionamento do WiFi

O usuário deve editar o arquivo "secrets.ini.example" para por as informações de rede, e depois renomea-lo para somente "secrets.ino". Desta forma o ESP irá coletar as informações de rede do usuário.
