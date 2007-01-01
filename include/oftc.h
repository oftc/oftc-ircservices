#ifndef OFTC_H
#define OFTC_H

#include <string>
#include <vector>

using std::vector;
using std::string;

class SIDMessage : public Message
{
public:
  SIDMessage() : Message("SID") {};
  ~SIDMessage() {};

  void handler(Server *, BaseClient *, vector<string>);
};

class UIDMessage : public Message
{
public:
  UIDMessage() : Message("UID") {};
  ~UIDMessage() {};

  void handler(Server *, BaseClient *, vector<string>);
};

class OFTCProtocol : public Protocol
{
public:
  // Constructors
  OFTCProtocol() : Protocol() {};
  OFTCProtocol(string const& n) : Protocol(n) {};
  
  ~OFTCProtocol() {};

  void init(Parser *, Connection *);
  void connected(bool=false);
  void introduce_client(Server *);
  void introduce_client(Client *);
private:
};

#endif
