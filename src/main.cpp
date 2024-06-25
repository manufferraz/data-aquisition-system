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
            std::string message(std::istreambuf_iterator<char>(is), {});
            std::cout << "Received: " << message << std::endl;
            
            //conversão de message para char
            char msg[message.length() + 1];
            std::strcpy(msg, message.c_str());

            // Processa a mensagem e cria um array de elementos
            char *aux[1027];
            int numTokens = 0;

            char temp[1027];
            strcpy(temp, msg); 
            char *token = strtok(temp, "|");
            while (token != NULL) {
                aux[numTokens++] = token;
                token = strtok(NULL, " ");
            }

            // Verifica o conteúdo da mensagem recebida 
            if (numTokens > 0 && strcmp(aux[0], "LOG") == 0) {                
              int sensorId, data_hora, leitura;

              for (int i = 1; i < numTokens; i++) {
                  if (i == 1) {
                      sscanf(aux[i], "%d", &sensorId);
                  } else if (i == 2) {
                      sscanf(aux[i], "%d", &data_hora);
                  } else if (i == 3) {
                      sscanf(aux[i], "%d", &leitura);
                  } 
              }
              char *resposta =  save_sensor(sensorId, data_hora, leitura);
              write_message(resposta);

            } else if (numTokens > 0 && strcmp(aux[0], "GET") == 0) {                
              int sensorId;

              for (int i = 1; i < numTokens; i++) {
                  if (i == 1) {
                    sscanf(aux[i], "%d", &sensorId);
                  }
                  else if(i == 2) {
                    sscanf(aux[i], "%d", &n_registros);
                  }
              }
              char *resposta =  send_data(sensorId, n_registros); //cliente manda mensagem, servidor pega as últimas medições e manda para o cliente
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
    int  sensor_id;
    //[30];
    char data_hora[30];
    char leitura[30]; 
  };

  // void save_sensor(char sensor_id, chat data_hora, char leitura)
  // {
  //   // Abre o arquivo para leitura e escrita em modo binário e coloca o apontador do arquivo
  //   // apontando para o fim de arquivo
  //   std::fstream file("sensor.dat", std::fstream::out | std::fstream::in | std::fstream::binary 
  //                                   | std::fstream::app); 
  //   // Caso não ocorram erros na abertura do arquivo
  //   if (file.is_open())
  //   {
  //     // Imprime a posição atual do apontador do arquivo (representa o tamanho do arquivo)
  //     int file_size = file.tellg();

  //     // Recupera o número de registros presentes no arquivo
  //     int n = file_size/sizeof(record_t);
  //     std::cout << "Num records: " << n << " (file size: " << file_size << " bytes)" << std::endl;

  //     // Escreve 10 registros no arquivo
  //     std::cout << "Writing 10 more records..." << std::endl;
  //     int id = n+1;
  //     for (unsigned i =0; i < 10; ++i)
  //     {
  //       record_t rec;
  //       rec.sensor_id = sensor_id;
  //       // rec.data_hora = data_hora;
  //       // rec.leitura = leitura;
  //       file.write((char*)&rec, sizeof(record_t));
  //     }

  //     // Imprime a posição atual do apontador do arquivo (representa o tamanho do arquivo)
  //     file_size = file.tellg();
  //     // Recupera o número de registros presentes no arquivo
  //     n = file_size/sizeof(record_t);
  //     std::cout << "Num records: " << n << " (file size: " << file_size << " bytes)" << std::endl;
      
  //     bool id_ok = false;
  //     while (!id_ok)
  //     {
  //       std::cout << "Id: ";
  //       std::cin >> id;
  //       if (id > n)
  //         std::cout << "Invalid id" << std::endl;
  //       else
  //         id_ok = true;
  //     }
  //     file.seekp((id-1)*sizeof(record_t), std::ios_base::beg);

  //     // Le o registro selecionado
  //     record_t rec;
  //     file.read((char*)&rec, sizeof(record_t));

  //     // Imprime o registro
  //     std::cout << "Id: "  << rec.sensor_id << " - Name: " << rec.data_hora << " - Phone: "<< rec.leitura << std::endl;

  //     // Fecha o arquivo
  //     file.close();
  //   }
  //   else
  //   {
  //     std::cout << "Error opening file!" << std::endl;
  //   }
   //}

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
