# ESP32-CAM
Vários projetos com o ESP32 CAM

### CameraWebServer:
É o projeto original de Fazer Stream com a câmera do ESP32 CAM.
</br>Minhas modificações:
- Traduzido os logs do Serial
- Removido configurações de outros modelos de câmera deixando apenas a CAMERA_MODEL_AI_THINKER
- EM BREVE: Remover a compactação da página web para possibilitar alteração na mesma.

### ESP32Cam_FotoReset:
Projeto Original de Rui Santos, os créditos estão no código. Ao apertar o botão de reset uma foto é tirada e armazenada no Cartão SD.
</br>Minhas modificações:
- Traduzido os logs do Serial

### ESP32Cam_TimeLapse:
Baseado no Projeto anterior de Rui Santos, neste programa você seleciona a quantidade de fotos e o tempo entre uma foto e outra, criando assim a função de TimeLapse.
</br>Minhas modificações:
 - Traduzido os logs do Serial
 - Função de TimeLapse
 - Código separado por funções(voids)
