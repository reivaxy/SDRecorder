#ifndef STYLES_H
#define STYLES_H

#include <Arduino.h>

// CSS styles for web interface
const char* RECORDER_CSS = R"(
  <style>
    * {
        margin: 0;
        padding: 0;
        box-sizing: border-box;
    }
    body {
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        min-height: 100vh;
        display: flex;
        justify-content: center;
        align-items: center;
        padding: 5px;
    }
    .container {
        background: white;
        border-radius: 10px;
        box-shadow: 0 10px 40px rgba(0,0,0,0.2);
        max-width: 600px;
        width: 100%;
        padding: 20px;
    }
    h1 {
        color: #333;
        text-align: center;
    }
    h2 {
        color: #333;
        margin-bottom: 12px;
        text-align: center;
    }
    .settings-section {
        margin-bottom: 30px;
    }
    .form-group {
        margin-bottom: 20px;
    }
    label {
        display: block;
        margin-bottom: 8px;
        color: #555;
        font-weight: 500;
    }
    input, select {
        width: 100%;
        padding: 10px;
        border: 2px solid #ddd;
        border-radius: 5px;
        font-size: 14px;
        transition: border-color 0.3s;
    }
    input:focus, select:focus {
        outline: none;
        border-color: #667eea;
    }
    button {
        width: 100%;
        padding: 12px;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: white;
        border: none;
        border-radius: 5px;
        font-size: 16px;
        font-weight: 600;
        cursor: pointer;
        transition: transform 0.2s;
    }
    button:hover {
        transform: translateY(-2px);
        box-shadow: 0 5px 20px rgba(102, 126, 234, 0.4);
    }
    button:active {
        transform: translateY(0);
    }
    .status {
        padding: 15px;
        border-radius: 5px;
        margin-top: 20px;
        display: none;
        text-align: center;
        font-weight: 600;
    }
    .status.success {
        background: #d4edda;
        color: #155724;
        display: block;
    }
    .status.error {
        background: #f8d7da;
        color: #721c24;
        display: block;
    }
    h2 {
        color: #333;
        margin-bottom: 20px;
        font-size: 18px;
    }
    .instruction-text {
        color: #666;
        margin: 0;
    }
    .instruction-text-bottom {
        color: #666;
        margin: 0 0 20px 0;
    }
    .device-status-text {
        color: #666;
        margin-bottom: 20px;
    }
    .device-name {
        font-weight: bold;
    }
    button + button {
        margin-top: 10px;
    }
    .checkbox-container {
        display: flex;
        align-items: center;
    }
    .checkbox-container input[type="checkbox"] {
        width: auto;
        margin-right: 10px;
        cursor: pointer;
        height: 18px;
        width: 18px;
    }
    .checkbox-container label {
        margin: 0;
        flex: 1;
        cursor: pointer;
    }
    .file-list {
        list-style: none;
        padding: 0;
        margin: 0;
    }
    .file-list li {
        padding: 10px 0;
        border-bottom: 1px solid #eee;
    }
    .file-list li:last-child {
        border-bottom: none;
    }
    .file-list a {
        color: #667eea;
        text-decoration: none;
        font-weight: 500;
        display: flex;
        align-items: center;
        gap: 8px;
    }
    .file-list a::before {
        content: '\1F3B5';
    }
    .file-list a:hover {
        color: #764ba2;
        text-decoration: underline;
    }
    .file-list li {
        display: flex;
        justify-content: space-between;
        align-items: center;
    }
    .file-size {
        color: #999;
        font-size: 13px;
        white-space: nowrap;
        margin-left: 12px;
    }
    .recording-indicator {
        display: inline-block;
        width: 12px;
        height: 12px;
        border-radius: 50%;
        background: #ccc;
        margin-right: 6px;
        vertical-align: middle;
    }
    .recording-indicator.active {
        background: #dc3545;
        animation: rec-blink 1s infinite;
    }
    @keyframes rec-blink {
        0%, 100% { opacity: 1; }
        50%       { opacity: 0.3; }
    }
    button.btn-success {
        background: linear-gradient(135deg, #28a745 0%, #20c997 100%);
    }
    button.btn-success:hover {
        box-shadow: 0 5px 20px rgba(40, 167, 69, 0.4);
    }
    button.btn-danger {
        background: linear-gradient(135deg, #dc3545 0%, #c82333 100%);
    }
    button.btn-danger:hover {
        box-shadow: 0 5px 20px rgba(220, 53, 69, 0.4);
    }
  </style>
)";

#endif // STYLES_H
