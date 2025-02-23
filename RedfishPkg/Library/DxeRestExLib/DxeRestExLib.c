/** @file
  This library provides help functions for REST EX Protocol.

  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NetLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Http.h>
#include <Protocol/RestEx.h>

#define REST_EX_CONFIG_DATA_LEN_UNKNOWN  0xff

/**
  This function allows the caller to create child handle for specific
  REST server.

  @param[in]  Controller           The controller handle used of selected interface.
  @param[in]  Image                The image handle used to open service.
  @param[in]  AccessMode           Access mode of REST server.
  @param[in]  ConfigType           Underlying configuration to communicate with REST server.
  @param[in]  ServiceType          REST service type.
  @param[out] ChildInstanceHandle  The handle to receive the create child.

  @retval  EFI_SUCCESS            Can't create the corresponding REST EX child instance.
  @retval  EFI_INVALID_PARAMETERS Any of input parameters is improper.

**/
EFI_STATUS
RestExLibCreateChild (
  IN EFI_HANDLE                       Controller,
  IN EFI_HANDLE                       Image,
  IN EFI_REST_EX_SERVICE_ACCESS_MODE  AccessMode,
  IN EFI_REST_EX_CONFIG_TYPE          ConfigType,
  IN EFI_REST_EX_SERVICE_TYPE         ServiceType,
  OUT EFI_HANDLE                      *ChildInstanceHandle
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                ChildHandle;
  EFI_REST_EX_PROTOCOL      *RestEx;
  EFI_REST_EX_SERVICE_INFO  *RestExServiceInfo;
  UINT8                     LenOfConfig;

  if ((Image == NULL) ||
      (AccessMode >= EfiRestExServiceModeMax) ||
      (ConfigType >= EfiRestExConfigTypeMax) ||
      (ServiceType >= EfiRestExServiceTypeMax) ||
      (ChildInstanceHandle == NULL)
      )
  {
    return EFI_INVALID_PARAMETER;
  }

  *ChildInstanceHandle = NULL;

  ChildHandle = NULL;
  Status = NetLibCreateServiceChild (
             Controller,
             Image,
             &gEfiRestExServiceBindingProtocolGuid,
             &ChildHandle
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to create service child - %r \n",
      __FUNCTION__,
      Status
      ));
    return Status;
  }
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiRestExProtocolGuid,
                  (VOID **)&RestEx,
                  Image,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Get the information of REST service provided by this EFI REST EX driver
  //
  Status = RestEx->GetService (
                     RestEx,
                     &RestExServiceInfo
                     );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }
  //
  // Check REST EX property.
  //
  switch (ConfigType) {
  case EfiRestExConfigHttp:
    LenOfConfig = sizeof (EFI_REST_EX_HTTP_CONFIG_DATA);
    break;

  case EfiRestExConfigUnspecific:
    LenOfConfig = REST_EX_CONFIG_DATA_LEN_UNKNOWN;
    break;

  default:
    goto ON_ERROR;
  }
  if (RestExServiceInfo->EfiRestExServiceInfoV10.RestServiceAccessMode != AccessMode ||
      RestExServiceInfo->EfiRestExServiceInfoV10.RestServiceType != ServiceType ||
      RestExServiceInfo->EfiRestExServiceInfoV10.RestExConfigType != ConfigType ||
      ((LenOfConfig != REST_EX_CONFIG_DATA_LEN_UNKNOWN) && (RestExServiceInfo->EfiRestExServiceInfoV10.RestExConfigDataLength != LenOfConfig))) {
    goto ON_ERROR;
  }

  //
  // This is proper REST EX instance.
  //
  *ChildInstanceHandle = ChildHandle;
  return EFI_SUCCESS;

ON_ERROR:;
    NetLibDestroyServiceChild (
      Controller,
      Image,
      &gEfiRestExServiceBindingProtocolGuid,
      ChildHandle
      );
  return EFI_NOT_FOUND;
}
