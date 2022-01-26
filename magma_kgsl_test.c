/*
 * Copyright 2021 Android Open Source Project
 * SPDX-License-Identifier: MIT
 */
#define _GNU_SOURCE
#include "magma.h"
#include "msm_kgsl.h"
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

static const char *kDevicePath = "/dev/kgsl";

struct magma_kgsl_connection {
	uint32_t contexts[4];
	magma_device_t device;
};

struct magma_kgsl_device {
	magma_handle_t device_channel;
	struct magma_kgsl_connection *connections[4];
};

magma_status_t magma_device_import(magma_handle_t device_channel, magma_device_t *device_out)
{
	struct magma_kgsl_device *kgsl = calloc(1, sizeof(struct magma_kgsl_device));
	kgsl->device_channel = device_channel;
	*device_out = (magma_device_t)kgsl;
	return 0;
}

magma_status_t magma_create_connection2(magma_device_t device, magma_connection_t *connection_out)
{
	struct magma_kgsl_device *kgsl = (struct magma_kgsl_device *)(device);
	struct magma_kgsl_connection *conn = calloc(1, sizeof(struct magma_kgsl_connection));
	kgsl->connections[0] = conn;
	conn->device = device;
	*connection_out = (magma_connection_t)conn;
	return 0;
}

void magma_release_connection(magma_connection_t connection)
{
	struct magma_kgsl_connection *conn = (struct magma_kgsl_connection *)(connection);
	struct magma_kgsl_device *kgsl = (struct magma_kgsl_device *)(conn->device);
	kgsl->connections[0] = NULL;
	free(conn);
	return;
}

void magma_device_release(magma_device_t device)
{
	struct magma_kgsl_device *kgsl = (struct magma_kgsl_device *)(device);
	close(kgsl->device_channel);
	return;
}

magma_status_t magma_create_context(magma_connection_t connection, uint32_t *context_id_out)
{
	struct kgsl_drawctxt_create req;
	struct magma_kgsl_connection *conn = (struct magma_kgsl_connection *)(connection);
	struct magma_kgsl_device *kgsl = (struct magma_kgsl_device *)(conn->device);

	int ret = ioctl(kgsl->device_channel, IOCTL_KGSL_DRAWCTXT_CREATE, &req);
	*context_id_out = req.drawctxt_id;
	return ret;
}

void magma_release_context(magma_connection_t connection, uint32_t context_id)
{
	struct magma_kgsl_connection *conn = (struct magma_kgsl_connection *)(connection);
	struct magma_kgsl_device *kgsl = (struct magma_kgsl_device *)(conn->device);
	struct kgsl_drawctxt_destroy req;
	req.drawctxt_id = context_id;

	int ret = ioctl(kgsl->device_channel, IOCTL_KGSL_DRAWCTXT_DESTROY, &req);
	return;
}

magma_status_t
magma_execute_immediate_commands2(magma_connection_t connection, uint32_t context_id,
				  uint64_t command_count,
				  struct magma_inline_command_buffer *command_buffers)
{
	struct magma_kgsl_connection *conn = (struct magma_kgsl_connection *)(connection);
	struct magma_kgsl_device *kgsl = (struct magma_kgsl_device *)(conn->device);
	int ret;

	for (uint32_t i = 0; i < command_count; i++) {
		struct kgsl_gpu_command command;
		command.context_id = context_id;
		command.cmdlist = (uint64_t)command_buffers[i].data;
		command.cmdsize = command_buffers[i].size;
		ret = ioctl(kgsl->device_channel, IOCTL_KGSL_GPU_COMMAND, &command);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int device_file_descriptor = open(kDevicePath, O_NONBLOCK);

	struct magma_inline_command_buffer buffer;
	char command_data[4];
	command_data[0] = 0xA;
	command_data[1] = 0xB;
	command_data[2] = 0xC;
	command_data[3] = 0xD;
	buffer.data = &command_data;
	buffer.size = 4;

	magma_device_t device;
	magma_connection_t connection;
	uint32_t context_id;

	magma_device_import(device_file_descriptor, &device);
	magma_create_connection2(device, &connection);

	magma_create_context(connection, &context_id);
	magma_execute_immediate_commands2(connection, context_id, 1, &buffer);

	magma_release_context(connection, context_id);
	magma_release_connection(connection);
	magma_device_release(device);
	return 0;
}
