#include <any>
#include <functional>
#include <optional>
#include <print>
#include <string>
#include <unordered_map>
#include <variant>

// Custom configuration structure to be stored inside std::any
struct CalibrationConfig {
    double offset;
    bool active;
};

// Variant representing possible metric output types from a sensor
using SensorMetric = std::variant<double, int, std::string>;

class ClimateStation {
    std::string station_id_;
    // Generic metadata repository using std::any (Type Erasure)
    std::unordered_map<std::string, std::any> metadata_;
    // Event callback for alerts using std::function
    std::function<void(const std::string&, double)> alert_callback_;

public:
    explicit ClimateStation(std::string id)
        : station_id_(std::move(id))
    {
    }

    // Store arbitrary configuration using std::any
    void set_metadata(const std::string& key, std::any value)
    {
        metadata_[key] = std::move(value);
    }

    // Register a custom alert listener via std::function
    void set_alert_callback(std::function<void(const std::string&, double)> callback)
    {
        alert_callback_ = std::move(callback);
    }

    // Process a raw temperature reading safely using std::optional
    std::optional<SensorMetric> process_temperature(std::optional<double> raw_temp)
    {
        // 1. std::optional check: Handle missing/failed sensor data
        if (!raw_temp.has_value()) {
            return std::nullopt;
        }

        double temp = raw_temp.value();

        // 2. Retrieve calibration from std::any metadata if available
        auto it = metadata_.find("calibration");
        if (it != metadata_.end()) {
            try {
                auto config = std::any_cast<CalibrationConfig>(it->second);
                if (config.active) {
                    temp += config.offset;
                }
            } catch (const std::bad_any_cast&) {
                std::println("[Warning] Invalid calibration configuration format!");
            }
        }

        // 3. Trigger alert callback if a critical threshold is reached
        if (temp > 40.0 && alert_callback_) {
            alert_callback_(station_id_, temp);
        }

        // 4. Return wrapped inside a std::variant
        return temp;
    }
};

int main()
{
    ClimateStation station("Amazon-Flux-Tower-04");

    // Setting up generic properties with std::any
    CalibrationConfig calibData { 1.2, true };
    station.set_metadata("calibration", calibData);
    station.set_metadata("ecosystem", std::string("Rainforest"));

    // Setting up an alert handler using std::function
    station.set_alert_callback([](const std::string& id, double temp) {
        std::println("[CRITICAL ALERT] Station {} detected extreme heat wave: {} °C!", id, temp);
    });

    // Simulating different sensor input scenarios with std::optional
    std::optional<double> normal_reading = 35.4;
    std::optional<double> heatwave_reading = 40.0; // 40.0 + 1.2 offset = 41.2 (Triggers alert!)
    std::optional<double> faulty_reading = std::nullopt;

    auto evaluate_reading = [&](std::optional<double> input) {
        auto result = station.process_temperature(input);

        // Checking the std::optional result
        if (result.has_value()) {
            // Safely inspecting the std::variant using std::visit
            std::visit([](auto&& arg) {
                std::println("Successfully processed metric value: {}", arg);
            },
                result.value());
        } else {
            std::println("-> Sensor Error: No data payload received (Signal lost).");
        }
    };

    std::println("=== Climatology Telemetry Processing Simulation ===");

    std::println("Test 1 (Normal Reading):");
    evaluate_reading(normal_reading);

    std::println("Test 2 (Extreme Heatwave Reading):");
    evaluate_reading(heatwave_reading);

    std::println("Test 3 (Sensor Failure/Missing Data):");
    evaluate_reading(faulty_reading);

    return 0;
}
