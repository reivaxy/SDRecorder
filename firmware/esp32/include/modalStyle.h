#ifndef MODAL_STYLE_H
#define MODAL_STYLE_H

#include <Arduino.h>

// CSS styles for modal dialogs
const char* MODAL_CSS = R"(
    <style>
        .modal {
            display: none;
            position: fixed;
            z-index: 1000;
            left: 0;
            top: 0;
            width: 100%;
            height: 100%;
            background-color: rgba(0,0,0,0.4);
        }
        
        .modal-content {
            background-color: #fefefe;
            margin: 15% auto;
            padding: 20px;
            border: 1px solid #888;
            border-radius: 8px;
            width: 80%;
            max-width: 300px;
            text-align: center;
        }
        
        .modal-content h2 {
            margin-top: 0;
            color: #333;
        }
        
        .modal-buttons {
            display: flex;
            gap: 10px;
            justify-content: center;
            margin-top: 20px;
        }
        
        .btn-modal {
            padding: 8px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
        }
        
        .btn-confirm {
            background-color: #d32f2f;
            color: white;
        }
        
        .btn-confirm:hover {
            background-color: #b71c1c;
        }
        
        .btn-cancel {
            background-color: #757575;
            color: white;
        }
        
        .btn-cancel:hover {
            background-color: #616161;
        }
        
        .danger-section {
            margin-top: 30px;
            padding-top: 20px;
            border-top: 2px solid #f0f0f0;
        }
        
        .danger-section h3 {
            color: #d32f2f;
        }
        
        .btn-danger-action {
            background-color: #d32f2f;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
            margin-top: 10px;
        }
        
        .btn-danger-action:hover {
            background-color: #b71c1c;
        }
    </style>
)";

#endif // MODAL_STYLE_H
