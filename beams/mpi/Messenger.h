#ifndef beams_mpi_Messenger_h
#define beams_mpi_Messenger_h

#include "MpiEnv.h"

#include <vtkm/Types.h>
#include <vtkm/thirdparty/diy/diy.h>

#include <list>
#include <map>
#include <memory>
#include <mpi.h>
#include <set>
#include <vector>

namespace beams
{
namespace mpi
{
class Messenger
{
public:
  VTKM_CONT Messenger(std::shared_ptr<MpiEnv> mpiEnv);
  VTKM_CONT virtual ~Messenger() { this->CleanupRequests(); }

  int GetRank() const { return this->Rank; }
  int GetNumRanks() const { return this->NumRanks; }

  VTKM_CONT void RegisterTag(int tag, std::size_t numRecvs, std::size_t size);

protected:
  static std::size_t CalcMessageBufferSize(std::size_t msgSz);

  void InitializeBuffers();
  void CheckPendingSendRequests();
  void CleanupRequests(int tag = TAG_ANY);
  void SendData(int dst, int tag, const vtkmdiy::MemoryBuffer& buff);
  bool RecvData(const std::set<int>& tags,
                std::vector<std::pair<int, vtkmdiy::MemoryBuffer>>& buffers,
                bool blockAndWait = false);

private:
  void PostRecv(int tag);
  void PostRecv(int tag, std::size_t sz, int src = -1);


  //Message headers.
  typedef struct
  {
    int rank, tag;
    std::size_t id, numPackets, packet, packetSz, dataSz;
  } Header;

  std::shared_ptr<beams::mpi::MpiEnv> Mpi;

  bool RecvData(int tag, std::vector<vtkmdiy::MemoryBuffer>& buffers, bool blockAndWait = false);

  void PrepareForSend(int tag, const vtkmdiy::MemoryBuffer& buff, std::vector<char*>& buffList);
  vtkm::Id GetMsgID() { return this->MsgID++; }
  static bool PacketCompare(const char* a, const char* b);
  void ProcessReceivedBuffers(std::vector<char*>& incomingBuffers,
                              std::vector<std::pair<int, vtkmdiy::MemoryBuffer>>& buffers);

  // Send/Recv buffer management structures.
  using RequestTagPair = std::pair<MPI_Request, int>;
  using RankIdPair = std::pair<int, int>;

  //Member data
  std::map<int, std::pair<std::size_t, std::size_t>> MessageTagInfo;
  MPI_Comm MPIComm;
  std::size_t MsgID;
  int NumRanks;
  int Rank;
  std::map<RequestTagPair, char*> RecvBuffers;
  std::map<RankIdPair, std::list<char*>> RecvPackets;
  std::map<RequestTagPair, char*> SendBuffers;
  static constexpr int TAG_ANY = -1;

  void CheckRequests(const std::map<RequestTagPair, char*>& buffer,
                     const std::set<int>& tags,
                     bool BlockAndWait,
                     std::vector<RequestTagPair>& reqTags);
};
}
} // beams::mpi

#endif // beams_mpi_Messenger_h