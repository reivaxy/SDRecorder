#ifndef FILES_HTML_PAGE_H
#define FILES_HTML_PAGE_H

const char FILES_HTML_PAGE_TEMPLATE[] = R"(
</head>
<body>
    <div class="container">
        <h1>SD Card Files</h1>
        <div class="settings-section">
%FILE_LIST%
        </div>
        <button onclick="location.href='/'">Home</button>
    </div>

    <!-- Delete Confirmation Modal -->
    <div id="deleteModal" class="modal">
        <div class="modal-content">
            <h2>Delete Files?</h2>
            <p id="deleteConfirmMessage">Are you sure you want to delete the selected files? This action cannot be undone.</p>
            <div class="modal-buttons">
                <button class="btn-modal btn-cancel" onclick='cancelDelete()'>Cancel</button>
                <button class="btn-modal btn-confirm" onclick='confirmDelete()'>Delete</button>
            </div>
        </div>
    </div>

    <script>
        function getSelectedFiles() {
            const checkboxes = document.querySelectorAll('.file-select:checked');
            return Array.from(checkboxes).map(cb => cb.value);
        }

        function updateSelectedCount() {
            const selected = getSelectedFiles();
            const selectAllCheckbox = document.getElementById('selectAllCheckbox');
            const allCheckboxes = document.querySelectorAll('.file-select');
            const deleteSection = document.getElementById('deleteSection');
            const selectAllContainer = document.getElementById('selectAllContainer');
            const selectedCount = document.getElementById('selectedCount');

            selectAllCheckbox.checked = selected.length === allCheckboxes.length && allCheckboxes.length > 0;
            selectAllCheckbox.indeterminate = selected.length > 0 && selected.length < allCheckboxes.length;

            if (selected.length > 0) {
                deleteSection.classList.add('visible');
                selectAllContainer.classList.add('visible');
            } else {
                deleteSection.classList.remove('visible');
                selectAllContainer.classList.remove('visible');
            }

            selectedCount.textContent = selected.length + (selected.length === 1 ? ' file selected' : ' files selected');
        }

        function toggleSelectAll() {
            const selectAllCheckbox = document.getElementById('selectAllCheckbox');
            const fileCheckboxes = document.querySelectorAll('.file-select');
            fileCheckboxes.forEach(cb => {
                cb.checked = selectAllCheckbox.checked;
            });
            updateSelectedCount();
        }

        function deleteSelectedFiles() {
            const selected = getSelectedFiles();
            if (selected.length === 0) {
                alert('No files selected');
                return;
            }

            const message = 'Are you sure you want to delete ' + selected.length + ' file' + (selected.length === 1 ? '' : 's') + '?';
            document.getElementById('deleteConfirmMessage').textContent = message;
            document.getElementById('deleteModal').style.display = 'block';
        }

        function cancelDelete() {
            document.getElementById('deleteModal').style.display = 'none';
        }

        function confirmDelete() {
            document.getElementById('deleteModal').style.display = 'none';
            const selected = getSelectedFiles();

            fetch('/apis/files/delete', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ files: selected })
            })
            .then(response => response.json())
            .then(data => {
                if (data.deleted > 0) {
                    let message = 'Successfully deleted ' + data.deleted + ' file' + (data.deleted === 1 ? '' : 's');
                    if (data.failed > 0) {
                        message += '. Failed to delete ' + data.failed + ' file' + (data.failed === 1 ? '' : 's');
                    }
                    alert(message);
                    location.reload();
                } else {
                    alert('Failed to delete files.');
                }
            })
            .catch(error => {
                console.error('Error deleting files:', error);
                alert('Error deleting files');
            });
        }

        window.addEventListener('load', function() {
            updateSelectedCount();
        });

        window.onclick = function(event) {
            const modal = document.getElementById('deleteModal');
            if (event.target == modal) {
                modal.style.display = 'none';
            }
        }
    </script>
</body>
</html>
)";

#endif // FILES_HTML_PAGE_H
