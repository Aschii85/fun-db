#include "nlohmann/json.hpp"
#include "../inc/db.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <stdexcept>
#include <optional>

// Function to handle a single client connection.
void handle_client(FunDB::Database database, int client_socket)
{
    char buffer[4096] = {0};
    read(client_socket, buffer, 4096);
    std::string request(buffer);

    std::stringstream ss(request);
    std::string method, path, http_version;
    ss >> method >> path >> http_version;

    std::string body;
    size_t body_start = request.find("\r\n\r\n");
    if (body_start != std::string::npos)
    {
        body = request.substr(body_start + 4);
    }

    std::string response_body;
    std::string status_line = "HTTP/1.1 200 OK\r\n";
    std::string content_type = "Content-Type: application/json\r\n";

    try
    {
        if (method == "POST")
        {
            // Use nlohmann/json to parse the request body
            nlohmann::json json_data = nlohmann::json::parse(body);

            if (path == "/store")
            {
                if (!json_data.contains("name") || !json_data.contains("expression") || !json_data.contains("symbols"))
                {
                    throw std::runtime_error("Missing 'name', 'symbols', or 'expression'.");
                }
                FunDB::Function func;
                func.name = json_data["name"].get<std::string>();
                func.expr = SymEngine::Expression(json_data["expression"].get<std::string>());

                for (const auto &symbol_json : json_data["symbols"])
                {
                    func.symbols.push_back(symbol_json.get<std::string>());
                }

                database.save_function(func);
                response_body = "{\"status\":\"Function stored successfully.\"}";
                status_line = "HTTP/1.1 201 Created\r\n";
            }
            else if (path == "/evaluate")
            {
                if (!json_data.contains("name") || !json_data.contains("values"))
                {
                    throw std::runtime_error("Missing 'name' or 'values'.");
                }
                std::unordered_map<std::string, double> values;
                for (auto const &[key, val] : json_data["values"].items())
                {
                    if (!val.is_number())
                    {
                        throw std::runtime_error("Invalid value format. Values must be numbers.");
                    }
                    values[key] = val.get<double>();
                }

                float result = FunDB::evaluate_stored_function(database, json_data["name"].get<std::string>(), values);
                response_body = "{\"result\":" + std::to_string(result) + "}";
                status_line = "HTTP/1.1 200 OK\r\n";
            }
            else
            {
                status_line = "HTTP/1.1 404 Not Found\r\n";
                response_body = "{\"error\":\"Path not found.\"}";
            }
        }
        else if (method == "GET")
        {
            if (path.rfind("/load/", 0) == 0)
            { // Check if path starts with "/load/"
                std::string name = path.substr(6);
                std::optional<FunDB::Function> func = database.load_function(name);

                if (func)
                {
                    std::ostringstream exprstr;
                    exprstr << func->expr;
                    nlohmann::json json_response;
                    json_response["name"] = func->name;
                    json_response["symbols"] = func->symbols;
                    json_response["expression"] = exprstr.str();
                    response_body = json_response.dump();
                }
                else
                {
                    status_line = "HTTP/1.1 404 Not Found\r\n";
                    response_body = "{\"error\":\"Function not found.\"}";
                }
            }
            else
            {
                status_line = "HTTP/1.1 404 Not Found\r\n";
                response_body = "{\"error\":\"Path not found.\"}";
            }
        }
        else
        {
            status_line = "HTTP/1.1 405 Method Not Allowed\r\n";
            response_body = "{\"error\":\"Method not allowed.\"}";
        }
    }
    catch (const nlohmann::json::parse_error &e)
    {
        status_line = "HTTP/1.1 400 Bad Request\r\n";
        response_body = "{\"error\":\"Invalid JSON format: " + std::string(e.what()) + "\"}";
    }
    catch (const std::exception &e)
    {
        status_line = "HTTP/1.1 400 Bad Request\r\n";
        response_body = "{\"error\":\"" + std::string(e.what()) + "\"}";
    }
    catch (...)
    {
        status_line = "HTTP/1.1 500 Internal Server Error\r\n";
        response_body = "{\"error\":\"An unknown error occurred.\"}";
    }

    std::string response = status_line + content_type + "Content-Length: " + std::to_string(response_body.length()) + "\r\n\r\n" + response_body;
    send(client_socket, response.c_str(), response.length(), 0);
    close(client_socket);
}

int main()
{
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int port = 6374;

    FunDB::Database database;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        std::cerr << "Socket creation error." << std::endl;
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "Bind failed." << std::endl;
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "Listen failed." << std::endl;
        return 1;
    }

    std::cout << "Server listening on port " << port << std::endl;

    while (true)
    {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            std::cerr << "Accept failed." << std::endl;
            continue;
        }
        handle_client(database, client_socket);
    }

    return 0;
}
