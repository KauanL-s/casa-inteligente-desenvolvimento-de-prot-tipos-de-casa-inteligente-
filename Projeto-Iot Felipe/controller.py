from devices import Device

class IoTController:
    def __init__(self):
        self.devices = {
            "1": Device("Lâmpada da Sala"),
            "2": Device("Ar-Condicionado"),
            "3": Device("Câmera de Segurança"),
            "4": Device("Computador"),
            "5": Device("Roteador Wi-Fi")
        }

    def show_dashboard(self):
        print("\n===== SISTEMA IoT – PAINEL DE STATUS =====")
        for key, device in self.devices.items():
            estado = device.status()
            visual = "[###]" if device.is_on else "[   ]"
            print(f"{key} - {device.name:<20} | {visual} {estado}")
        print("============================================\n")

    def toggle_device(self, device_id):
        if device_id in self.devices:
            device = self.devices[device_id]
            if device.is_on:
                device.turn_off()
                print(f"{device.name} foi DESLIGADO.")
            else:
                device.turn_on()
                print(f"{device.name} foi LIGADO.")
        else:
            print("ID inválido! Tente novamente.")
