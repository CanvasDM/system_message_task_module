#
# Copyright (c) 2022 Laird Connectivity LLC
#
# SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
#
if(CONFIG_SYSTEM_MESSAGE_TASK)
    add_fwk_id_file(${CMAKE_CURRENT_SOURCE_DIR}/framework_config/fwk_ids.h)
    zephyr_include_directories(include)
    zephyr_sources(source/system_message_task.c)
endif()
