#include <activemq/library/ActiveMQCPP.h>
#include <iostream>

#include "include/configuration/ContainerSetup.hpp"
#include "include/configuration/RunConfiguration.hpp"

int main() {
    std::cout << "RUNNING" << std::endl;
    activemq::library::ActiveMQCPP::initializeLibrary();
    const auto container = config::containerSetup();
    crow::SimpleApp app;

    // Bind all annotated routes
    for (auto& def : routeRegistry()) {
        def.binder(app, container);
    }

    auto appConfig = container->resolve<config::RunConfiguration>();

    // Imprimir puerto y URL base para facilitar pruebas desde Postman
    std::cout << "Listening on port: " << appConfig->port << std::endl;
    std::cout << "Base URL: http://localhost:" << appConfig->port << std::endl;

    app.port(appConfig->port)
        .concurrency(appConfig->concurrency)
        .run();
    activemq::library::ActiveMQCPP::shutdownLibrary();
}
