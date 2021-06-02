#pragma once

#include <stdint.h>
#include "Event.h"

#ifndef WM_COMMAND_TYPE
#define WM_COMMAND_TYPE uint32_t
#endif

typedef struct {
    WM_COMMAND_TYPE command; // always set to WM_WINDOW_CREATE
    uint32_t top_left_x;
    uint32_t top_left_y;
    uint32_t width;
    uint32_t height;
    const char* title; // NULL -> "Untitled"

    // A pointer to an array of 32 bpp RBGA pixels,
    // but not necessarily exactly width * height in length.
    // To calculate the Y offset for any row use the pitch by doing Y * pitch.
    // Set by the OS on success, NULL otherwise.
    void* framebuffer;

    // Amount of bytes for each Y row (not guaranteed to be equal to width * bpp), set by the OS.
    uint32_t pitch;

    uint32_t window_id; // Unique window ID set by the OS on success, 0 otherwise.
} WindowCreateCommand;

typedef struct {
    WM_COMMAND_TYPE command; // always set to WM_WINDOW_DESTROY
    uint32_t window_id;
} WindowDestroyCommand;

typedef struct {
    WM_COMMAND_TYPE command; // always set to WM_POP_EVENT
    uint32_t window_id;
    Event event; // Set by the OS, if internal event buffer is empty the event.type is set to ET_EMPTY.
} PopEventCommand;

typedef struct {
    WM_COMMAND_TYPE command; // always set to WM_SET_TITLE
    uint32_t window_id;
    const char* title;
} SetTitleCommand;

typedef struct {
    WM_COMMAND_TYPE command; // always set to WM_INVALIDATE_RECT
    uint32_t window_id;
    uint32_t top_left_x;
    uint32_t top_left_y;

    // If both width & height are 0, the command is interpreted as invalidate the entire rect.
    // If width is zero and height is not or vise versa INVALID_ARGUMENT is returned and nothing
    // is invalidated.
    uint32_t width;
    uint32_t height;
} InvalidateRectCommand;
