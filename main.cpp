#include <Windows.h>
#include <BlackBone/Process/Process.h>
#include <BlackBone/Patterns/PatternSearch.h>

static constexpr auto target = L"EU4.exe";

using namespace blackbone;
Process proc;

#pragma pack(push, 1)
// 48 89 2D ? ? ? ? 48 8B 1D ? ? ? ? 48 89 2D
struct CAchievementsManager {
  bool _bMultiplayer;
  bool _bSaveGameOK;
  bool _bGameOK;
  bool _bIsDebug;
};

// 48 89 3D ? ? ? ? 88 5F 19
struct CConsoleCmdManager {
  unsigned char padding_0x0000[0x18];
  bool _isRelease;
  // also means you're in ironman for some reason
  bool _bIsMultiplayer;
};
#pragma pop

uintptr_t GetAchievementsManager() {
  const PatternSearch ps{0x48, 0x89, 0x2D, 0xCC, 0xCC, 0xCC, 0xCC, 0x48, 0x8B, 0x1D, 0xCC, 0xCC, 0xCC, 0xCC, 0x48, 0x89, 0x2D};
  std::vector<ptr_t> out;
  if (ps.SearchRemote(proc, 0xCC, proc.modules().GetMainModule()->baseAddress, proc.modules().GetMainModule()->size, out)) {
    const auto g_AchievementsManager = out.front() + 7 + proc.memory().Read<DWORD>(out.front() + 3).result();
    printf("g_AchievementsManager: 0x%p [ %p ]\n", reinterpret_cast<void*>(g_AchievementsManager), reinterpret_cast<void*>(g_AchievementsManager - proc.modules().GetMainModule()->baseAddress));
    return proc.memory().Read<uintptr_t>(g_AchievementsManager).result();
  }
  return 0;
}

uintptr_t GetConsoleCmdManager() {
  const PatternSearch ps{0x48, 0x89, 0x3D, 0xCC, 0xCC, 0xCC, 0xCC, 0x88, 0x5F, 0x19};
  std::vector<ptr_t> out;
  if (ps.SearchRemote(proc, 0xCC, proc.modules().GetMainModule()->baseAddress, proc.modules().GetMainModule()->size, out)) {
    const auto g_ConsoleCmdManager = out.front() + 7 + proc.memory().Read<DWORD>(out.front() + 3).result();
    printf("g_ConsoleCmdManager: 0x%p [ %p ]\n", reinterpret_cast<void*>(g_ConsoleCmdManager), reinterpret_cast<void*>(g_ConsoleCmdManager - proc.modules().GetMainModule()->baseAddress));
    return proc.memory().Read<uintptr_t>(g_ConsoleCmdManager).result();
  }
  return 0;
}

int main() {
  blackbone::InitializeOnce();

  DWORD eu4_process_id;

  while (true) {
    auto found = Process::EnumByName(target);
    if (!found.empty()) {
      eu4_process_id = found.front();
      break;
    }
  }

  proc.Attach(eu4_process_id, PROCESS_ALL_ACCESS);

  if (proc.core().isWow64()) {
    printf("Not compatible EU4 version detected! Consider updating and re-run tool.\n");
    proc.Detach();
    return STATUS_UNSUCCESSFUL;
  }

  const auto main_module = proc.modules().GetMainModule();
  const auto base = main_module->baseAddress;

  if (const auto g_AchievementsManager = GetAchievementsManager()) {
    proc.memory().Write<byte>(g_AchievementsManager + offsetof(CAchievementsManager, _bMultiplayer), false);
    proc.memory().Write<byte>(g_AchievementsManager + offsetof(CAchievementsManager, _bSaveGameOK), true);
    proc.memory().Write<byte>(g_AchievementsManager + offsetof(CAchievementsManager, _bGameOK), true);
    proc.memory().Write<byte>(g_AchievementsManager + offsetof(CAchievementsManager, _bIsDebug), true);
  }

  if (const auto g_ConsoleCmdManager = GetConsoleCmdManager()) {
    proc.memory().Write<byte>(g_ConsoleCmdManager + offsetof(CConsoleCmdManager, _bIsMultiplayer), false);
    proc.memory().Write<byte>(g_ConsoleCmdManager + offsetof(CConsoleCmdManager, _isRelease), false);
  }

  printf("\n");
  printf("\n");
  printf("Successfully patched game!\n");
  printf("If you encounter any issues please report at:\n");
  printf("https://github.com/macho105/eu4_ironman_fix/issues\n");
  printf("\n");
  printf("If tool doesn't seem to work, please re-run it and check again!\n");
  printf("Some game versions might require to run tool after loading save or starting new game.\n");
  printf("\n");
  printf("\n");

  proc.Detach();
  system("pause");
  return 0;
}
