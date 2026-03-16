#ifndef FILES_STYLE_H
#define FILES_STYLE_H

#include <Arduino.h>

// CSS styles for files management page
const char* FILES_CSS = R"(
  <style>
    .file-list-container {
        margin-bottom: 20px;
    }
    .file-item {
        display: flex;
        align-items: center;
        padding: 2px;
        border-bottom: 1px solid #ddd;
    }
    .file-checkbox {
        margin-right: 15px;
        cursor: pointer;
        width: 20px;
        height: 20px;
    }
    .file-info {
        flex-grow: 1;
        display: flex;
        justify-content: space-between;
        align-items: center;
    }
    .file-link {
        color: #0066cc;
        text-decoration: none;
    }
    .file-link:hover {
        text-decoration: underline;
    }
    .file-size {
        color: #666;
        font-size: 0.7em;
        margin-left: 10px;
        white-space: nowrap;
    }
    .file-date {
        color: #999;
        font-size: 0.7em;
        margin-left: 10px;
        white-space: nowrap;
    }
    .delete-section {
        margin: 20px 0;
        padding: 15px;
        background-color: #f5f5f5;
        border-radius: 4px;
        display: none;
    }
    .delete-section.visible {
        display: block;
    }
    .select-all-container {
        padding: 10px;
        background-color: #f9f9f9;
        border-bottom: 1px solid #ddd;
        display: none;
    }
    .select-all-container.visible {
        display: flex;
        align-items: center;
    }
    .btn-delete-files {
        background-color: #dc3545;
        color: white;
        padding: 8px 16px;
        border: none;
        border-radius: 4px;
        cursor: pointer;
        font-size: 14px;
    }
    .btn-delete-files:hover {
        background-color: #c82333;
    }
    .selected-count {
        margin-left: 10px;
        color: #666;
    }
  </style>
)";

#endif
