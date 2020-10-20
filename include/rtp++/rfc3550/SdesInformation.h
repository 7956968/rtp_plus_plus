#pragma once

namespace rtp_plus_plus
{
namespace rfc3550
{

class SdesInformation
{
public:

  SdesInformation()
  {
  }

  SdesInformation(const std::string& sCName)
    : CName(sCName)
  {
  }

  std::string getCName() const { return CName; }
  void setCName(std::string val) { CName = val; }
  std::string getName() const { return Name; }
  void setName(std::string val) { Name = val; }
  std::string getEmail() const { return Email; }
  void setEmail(std::string val) { Email = val; }
  std::string getPhone() const { return Phone; }
  void setPhone(std::string val) { Phone = val; }
  std::string getLoc() const { return Loc; }
  void setLoc(std::string val) { Loc = val; }
  std::string getTool() const { return Tool; }
  void setTool(std::string val) { Tool = val; }
  std::string getNote() const { return Note; }
  void setNote(std::string val) { Note = val; }
  std::string getPriv() const { return Priv; }
  void setPriv(std::string val) { Priv = val; }

private:

  std::string CName;
  std::string Name;
  std::string Email;
  std::string Phone;
  std::string Loc;
  std::string Tool;
  std::string Note;
  std::string Priv;

};

}
}
