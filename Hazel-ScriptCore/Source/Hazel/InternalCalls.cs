using System;
using System.Runtime.CompilerServices;

namespace Hazel
{
	public static class InternalCalls
	{
		#region Entity
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static bool Entity_HasComponent(ulong entityID, Type componentType);
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static ulong Entity_FindEntityByName(string name);
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static object GetScriptInstance(ulong entityID);
		#endregion

		#region TransformComponent
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_GetTranslation(ulong entityID, out Vector3 translation);
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_SetTranslation(ulong entityID, ref Vector3 translation);
		#endregion

		#region SystemInternal
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
		internal extern static bool Input_IsKeyDown(KeyCode keycode);
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void NativeLog(System.String logString, int parameter);
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void NativeLog_Vector(ref Vector3 parameter, out Vector3 result);
		[MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static float NativeLog_VectorDot(ref Vector3 parameter);
		#endregion
	}
}
