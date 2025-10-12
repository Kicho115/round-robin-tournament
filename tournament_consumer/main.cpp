//
// Created by tomas on 9/6/25.
//
#include <activemq/library/ActiveMQCPP.h>
#include <thread>
#include <iostream>
#include <exception>

#include "configuration/ContainerSetup.hpp"
// Asegúrate que la ruta sea correcta en tu proyecto.
// En el resto del código se usan headers bajo "cms/..." para el bus.
#include "cms/QueueMessageConsumer.hpp"

int main() {
    try {
        activemq::library::ActiveMQCPP::initializeLibrary();

        std::cout << "before container" << std::endl;
        const auto container = config::containerSetup();
        std::cout << "after container" << std::endl;

        // Captura por valor para asegurar que el contenedor vive dentro del hilo.
        std::thread tournamentCreatedThread([container] {
            auto listener = container->resolve<QueueMessageConsumer>();
            listener->Start("tournament.created");
        });

        tournamentCreatedThread.join();

        // Si prefieres mantener el proceso vivo:
        // while (true) {
        //     std::this_thread::sleep_for(std::chrono::seconds(5));
        // }

        activemq::library::ActiveMQCPP::shutdownLibrary();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[FATAL] exception: " << ex.what() << std::endl;
        try { activemq::library::ActiveMQCPP::shutdownLibrary(); } catch (...) {}
        return 1;
    } catch (...) {
        std::cerr << "[FATAL] unknown exception" << std::endl;
        try { activemq::library::ActiveMQCPP::shutdownLibrary(); } catch (...) {}
        return 1;
    }
}
