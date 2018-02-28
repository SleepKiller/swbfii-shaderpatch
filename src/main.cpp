
#include "com_ptr.hpp"
#include "direct3d/check_required_features.hpp"
#include "direct3d/create_device.hpp"
#include "hook_vtable.hpp"
#include "input_hooker.hpp"

#include <exception>

#include <d3d9.h>
#include <dinput.h>

namespace {
using namespace sp;

HMODULE load_dinput_dll() noexcept
{
   const static auto handle = LoadLibraryW(LR"(C:\Windows\SysWOW64\DINPUT8.dll)");

   if (handle == nullptr) std::terminate();

   return handle;
}

template<typename Func, typename Func_ptr = std::add_pointer_t<Func>>
Func_ptr get_dinput_export(const char* name)
{
   const static auto lib_handle = load_dinput_dll();

   const auto proc = reinterpret_cast<Func_ptr>(GetProcAddress(lib_handle, name));

   if (proc == nullptr) std::terminate();

   return proc;
}

void hook_direct3d() noexcept
{
   Com_ptr<IDirect3D9> d3d{Direct3DCreate9(D3D_SDK_VERSION)};

   if (!d3d) std::terminate();

   direct3d::create_device =
      hook_vtable<direct3d::Create_type>(*d3d, 16, direct3d::create_device_hook);
}

void check_direct3d() noexcept
{
   Com_ptr<IDirect3D9> d3d{Direct3DCreate9(D3D_SDK_VERSION)};

   direct3d::check_required_features(*d3d);
}
}

extern "C" HRESULT __stdcall directinput8_create(HINSTANCE instance, DWORD version,
                                                 REFIID iid, LPVOID* out,
                                                 LPUNKNOWN unknown_outer)
{
   hook_direct3d();
   check_direct3d();

   using DirectInput8Create_Proc =
      HRESULT __stdcall(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

   const auto directinput_create =
      get_dinput_export<DirectInput8Create_Proc>("DirectInput8Create");

   const auto result = directinput_create(instance, version, iid, out, unknown_outer);

   initialize_input_hooks(GetCurrentThreadId(), *static_cast<IDirectInput8A*>(*out));

   return result;
}
