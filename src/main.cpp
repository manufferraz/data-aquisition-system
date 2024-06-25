#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    read_message();
  }

private:
void read_message()
    {
        auto self(shared_from_this());
        boost::asio::async_read_until(socket_, buffer_, "\r\n",
            [this, self](boost::system::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    std::istream is(&buffer_);
                    std::string message((std::istreambuf_iterator<char>(is)), {});
                    std::cout << "Received: " << message << std::endl;

                    // Processa a mensagem e cria um array de elementos
                    char *aux[1027];
                    int numTokens = 0;

                    char temp[1027];
                    std::strcpy(temp, message.c_str());
                    char *token = std::strtok(temp, "|");
                    while (token != nullptr) {
                        aux[numTokens++] = token;
                        token = std::strtok(nullptr, "|");
                    }

                    // Verifica o conteúdo da mensagem recebida
                    if (numTokens > 0 && std::strcmp(aux[0], "LOG") == 0) {
                        char sensor_id[20];
                        char data_hora[20];
                        char leitura[10];
                        std::cout << "Teste: " << aux[0] << std::endl;

                        if (numTokens >= 4) {
                            std::strcpy(sensor_id, aux[1]);
                            std::strcpy(data_hora, aux[2]);
                            std::strcpy(leitura, aux[3]);
                        }

                        std::string resposta = save_sensor(sensor_id, data_hora, leitura);
                        write_message(resposta);
                    } else if (numTokens > 0 && std::strcmp(aux[0], "GET") == 0) {
                        int n_registros = 0;
                        std::cout << "Teste: " << aux[0] << std::endl;

                        if (numTokens >= 3) {
                            n_registros = std::stoi(aux[2]);
                        }

                        std::string resposta = send_data(n_registros);
                        write_message(resposta);
                    }
                }
        });
  }
        

  void write_message(const std::string& message)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(message),
        [this, self, message](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            read_message();
          }
        });
  }

  struct record_t
  {
    char sensor_id[30];
    char data_hora[30];
    char leitura[30]; 
  };

  std::string save_sensor(const char* sensor_id, const char* data_hora, const char* leitura)
    {
        // Abre o arquivo para leitura e escrita em modo binário e coloca o apontador do arquivo
        // apontando para o fim de arquivo
        std::fstream file("sensor.dat", std::fstream::out | std::fstream::in | std::fstream::binary | std::fstream::app); 
        
        // Caso não ocorram erros na abertura do arquivo
        if (file.is_open())
        {
            // Escreve um novo registro no arquivo
            record_t rec;
            std::strncpy(rec.sensor_id, sensor_id, sizeof(rec.sensor_id) - 1);
            rec.sensor_id[sizeof(rec.sensor_id) - 1] = '\0';
            std::strncpy(rec.data_hora, data_hora, sizeof(rec.data_hora) - 1);
            rec.data_hora[sizeof(rec.data_hora) - 1] = '\0';
            std::strncpy(rec.leitura, leitura, sizeof(rec.leitura) - 1);
            rec.leitura[sizeof(rec.leitura) - 1] = '\0';
            file.write(reinterpret_cast<char*>(&rec), sizeof(record_t));
            
            // Fecha o arquivo
            file.close();

            return "LOG_SUCCESS\r\n";
        }
        else
        {
            std::cout << "Error opening file!" << std::endl;
            return "LOG_FAIL\r\n";
        }
    }

  std::string send_data(int n_registros)
{
    // Abre o arquivo para leitura em modo binário
    std::fstream file("sensor.dat", std::fstream::in | std::fstream::binary);
    
    // Caso não ocorram erros na abertura do arquivo
    if (file.is_open())
    {
        // Posiciona o apontador de leitura no final do arquivo para obter o tamanho
        file.seekg(0, std::ios::end);
        int file_size = file.tellg();

        // Recupera o número de registros presentes no arquivo
        int n = file_size / sizeof(record_t);
        std::cout << "Num records: " << n << " (file size: " << file_size << " bytes)" << std::endl;

        // Posiciona o apontador de leitura no início do arquivo
        file.seekg(0, std::ios::beg);

        // Lê todos os registros do arquivo
        std::vector<record_t> registros(n);
        file.read(reinterpret_cast<char*>(registros.data()), n * sizeof(record_t));

        // Fecha o arquivo
        file.close();

        // Monta a resposta
        std::ostringstream resposta;
        resposta << n;

        int start = std::max(0, n - n_registros);
        for (int i = start; i < n; ++i) {
            resposta << ';' << registros[i].data_hora << '|' << registros[i].leitura;
        }
        resposta << "\r\n";

        return resposta.str();
    }
    else
    {
        std::cout << "Error opening file!" << std::endl;
        return "Error opening file!\r\n";
    }
}




  tcp::socket socket_;
  boost::asio::streambuf buffer_;
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    accept();
  }

private:
  void accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();
          }

          accept();
        });
  }

  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    std::cerr << "Usage: chat_server <port>\n";
    return 1;
  }

  boost::asio::io_context io_context;

  server s(io_context, std::atoi(argv[1]));

  io_context.run();

  return 0;
}
