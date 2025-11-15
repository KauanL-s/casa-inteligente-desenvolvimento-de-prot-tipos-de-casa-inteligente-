from controle import IoTController

def menu():
    print("===== MENU PRINCIPAL =====")
    print("1 - Exibir painel IoT")
    print("2 - Alterar estado de um dispositivo")
    print("3 - Sair")
    return input("Escolha uma opção: ")

def main():
    system = IoTControle()
    running = True

    while running:
        choice = menu()

        if choice == "1":
            system.show_dashboard()

        elif choice == "2":
            system.show_dashboard()
            device_id = input("Digite o ID do dispositivo que deseja alternar: ")
            system.toggle_device(device_id)

        elif choice == "3":
            print("Encerrando o sistema IoT...")
            running = False

        else:
            print("Opção inválida!")

if __name__ == "__main__":
    main()
