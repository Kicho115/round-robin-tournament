
//
#include <activemq/library/ActiveMQCPP.h>
#include <cms/TextMessage.h>
#include <thread>
#include <iostream>
#include <exception>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

#include "configuration/ContainerSetup.hpp"
#include "consumer/MatchGenerationConsumer.hpp"
#include "consumer/ScoreProcessingConsumer.hpp"
#include "cms/ConnectionManager.hpp"

// Wrapper consumer for MatchGenerationConsumer
class MatchGenQueueListener : public cms::MessageListener {
    std::shared_ptr<MatchGenerationConsumer> consumer;
public:
    explicit MatchGenQueueListener(std::shared_ptr<MatchGenerationConsumer> consumer) 
        : consumer(std::move(consumer)) {}
    
    void onMessage(const cms::Message* message) override {
        try {
            if (auto text = dynamic_cast<const cms::TextMessage*>(message)) {
                std::string payload = text->getText();
                std::cout << "[MatchGenConsumer] Received message: " << payload << std::endl;
                
                // Parse JSON and call Handle
                auto eventJson = nlohmann::json::parse(payload);
                consumer->Handle(eventJson);
            }
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "[MatchGenConsumer] JSON parsing error: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[MatchGenConsumer] Error processing message: " << e.what() << std::endl;
        }
    }
};

// Wrapper consumer for ScoreProcessingConsumer
class ScoreProcessQueueListener : public cms::MessageListener {
    std::shared_ptr<ScoreProcessingConsumer> consumer;
public:
    explicit ScoreProcessQueueListener(std::shared_ptr<ScoreProcessingConsumer> consumer) 
        : consumer(std::move(consumer)) {}
    
    void onMessage(const cms::Message* message) override {
        try {
            if (auto text = dynamic_cast<const cms::TextMessage*>(message)) {
                std::string payload = text->getText();
                std::cout << "[ScoreProcessConsumer] Received message: " << payload << std::endl;
                
                // Parse JSON and call Handle
                auto eventJson = nlohmann::json::parse(payload);
                consumer->Handle(eventJson);
            }
        } catch (const nlohmann::json::exception& e) {
            std::cerr << "[ScoreProcessConsumer] JSON parsing error: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[ScoreProcessConsumer] Error processing message: " << e.what() << std::endl;
        }
    }
};

int main() {
    try {
        std::cout << "[Consumer] Starting tournament consumer service..." << std::endl;
        
        // 1. Initialize ActiveMQ library
        activemq::library::ActiveMQCPP::initializeLibrary();
        std::cout << "[Consumer] ActiveMQ library initialized" << std::endl;

        // 2. Set up dependency injection container
        std::cout << "[Consumer] Setting up dependency injection container..." << std::endl;
        auto container = config::containerSetup();
        std::cout << "[Consumer] Container setup complete" << std::endl;

        // 3. Resolve dependencies
        auto connectionManager = container->resolve<ConnectionManager>();
        auto matchGenConsumer = container->resolve<MatchGenerationConsumer>();
        auto scoreConsumer = container->resolve<ScoreProcessingConsumer>();

        std::cout << "[Consumer] Dependencies resolved" << std::endl;

        // 4. Create message listeners
        auto matchGenListener = std::make_shared<MatchGenQueueListener>(matchGenConsumer);
        auto scoreListener = std::make_shared<ScoreProcessQueueListener>(scoreConsumer);

        // 5. Create sessions and subscribe to topics
        auto matchGenSession = connectionManager->CreateSession();
        auto scoreSession = connectionManager->CreateSession();

        // Subscribe to group.team_added for match generation
        auto teamAddedQueue = std::unique_ptr<cms::Queue>(
            matchGenSession->createQueue("tournament.group.team_added")
        );
        auto matchGenMsgConsumer = std::unique_ptr<cms::MessageConsumer>(
            matchGenSession->createConsumer(teamAddedQueue.get())
        );
        matchGenMsgConsumer->setMessageListener(matchGenListener.get());

        std::cout << "[Consumer]  Subscribed to: tournament.group.team_added" << std::endl;

        // Subscribe to match.score_updated for score processing
        auto scoreUpdatedQueue = std::unique_ptr<cms::Queue>(
            scoreSession->createQueue("tournament.match.score_updated")
        );
        auto scoreMsgConsumer = std::unique_ptr<cms::MessageConsumer>(
            scoreSession->createConsumer(scoreUpdatedQueue.get())
        );
        scoreMsgConsumer->setMessageListener(scoreListener.get());

        std::cout << "[Consumer] Subscribed to: tournament.match.score_updated" << std::endl;

        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "   Tournament Consumer Service Ready" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "\nActive consumers:" << std::endl;
        std::cout << "  - MatchGenerationConsumer - tournament.group.team_added" << std::endl;
        std::cout << "  - ScoreProcessingConsumer - tournament.match.score_updated" << std::endl;
        std::cout << "\nWaiting for events...\n" << std::endl;

        // 6. Keep process alive
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(60));
            std::cout << "[Consumer] Service running... (uptime check)" << std::endl;
        }

        // 7. Cleanup (never reached in normal operation)
        matchGenMsgConsumer->close();
        scoreMsgConsumer->close();
        matchGenSession->close();
        scoreSession->close();
        
        activemq::library::ActiveMQCPP::shutdownLibrary();
        return 0;

    } catch (const cms::CMSException& e) {
        std::cerr << "\n[FATAL] ActiveMQ error: " << e.what() << std::endl;
        std::cerr << "Stack trace: " << e.getStackTraceString() << std::endl;
        try { activemq::library::ActiveMQCPP::shutdownLibrary(); } catch (...) {}
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "\n[FATAL] Exception: " << ex.what() << std::endl;
        try { activemq::library::ActiveMQCPP::shutdownLibrary(); } catch (...) {}
        return 1;
    } catch (...) {
        std::cerr << "\n[FATAL] Unknown exception" << std::endl;
        try { activemq::library::ActiveMQCPP::shutdownLibrary(); } catch (...) {}
        return 1;
    }
}
