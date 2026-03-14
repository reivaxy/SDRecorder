import sys, os, re, wave
from PyQt6.QtWidgets import (
    QApplication, QWidget, QPushButton, QLabel, QFileDialog, QVBoxLayout,
    QListWidget, QProgressBar, QMessageBox
)
from PyQt6.QtCore import Qt, QThread, pyqtSignal

class MergeThread(QThread):
    progress = pyqtSignal(int)
    finished = pyqtSignal(str)
    error = pyqtSignal(str)

    def __init__(self, files, output):
        super().__init__()
        self.files = files
        self.output = output

    def run(self):
        try:
            with wave.open(self.files[0], "rb") as first:
                params = first.getparams()

            with wave.open(self.output, "wb") as out:
                out.setparams(params)

                for i, f in enumerate(self.files):
                    with wave.open(f, "rb") as w:
                        if w.getparams()[:3] != params[:3]:
                            raise ValueError(f"Incompatible WAV: {f}")
                        out.writeframes(w.readframes(w.getnframes()))
                    self.progress.emit(i + 1)

            self.finished.emit(self.output)

        except Exception as e:
            self.error.emit(str(e))

class MergeWavApp(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Merge WAV Files (PyQt6)")
        self.folder = ""
        self.output_file = "merge.wav"
        self.wav_files = []

        layout = QVBoxLayout()

        self.btn_folder = QPushButton("Choose WAV Folder")
        self.btn_folder.clicked.connect(self.choose_folder)
        layout.addWidget(self.btn_folder)

        self.label_folder = QLabel("No folder selected")
        layout.addWidget(self.label_folder)

        self.btn_output = QPushButton("Choose Output File")
        self.btn_output.clicked.connect(self.choose_output)
        layout.addWidget(self.btn_output)

        self.label_output = QLabel("merge.wav (default)")
        layout.addWidget(self.label_output)

        self.list_widget = QListWidget()
        layout.addWidget(self.list_widget)

        self.progress = QProgressBar()
        layout.addWidget(self.progress)

        self.btn_merge = QPushButton("Merge WAV Files")
        self.btn_merge.clicked.connect(self.start_merge)
        layout.addWidget(self.btn_merge)

        self.setLayout(layout)

    def choose_folder(self):
        folder = QFileDialog.getExistingDirectory(self, "Select WAV folder")
        if folder:
            self.folder = folder
            self.label_folder.setText(folder)
            self.scan_files()

    def choose_output(self):
        path, _ = QFileDialog.getSaveFileName(self, "Output file", "merge.wav", "WAV files (*.wav)")
        if path:
            self.output_file = path
            self.label_output.setText(path)

    def scan_files(self):
        self.wav_files = []
        self.list_widget.clear()
        pattern = re.compile(r"rec(\d+)\.wav$", re.IGNORECASE)

        for f in os.listdir(self.folder):
            m = pattern.match(f)
            if m:
                self.wav_files.append((int(m.group(1)), os.path.join(self.folder, f)))

        self.wav_files.sort(key=lambda x: x[0])
        for _, f in self.wav_files:
            self.list_widget.addItem(f)

        self.progress.setValue(0)
        self.progress.setMaximum(len(self.wav_files))

    def start_merge(self):
        if not self.wav_files:
            QMessageBox.warning(self, "Error", "No recX.wav files found")
            return

        files = [f for _, f in self.wav_files]
        self.thread = MergeThread(files, self.output_file)
        self.thread.progress.connect(self.progress.setValue)
        self.thread.finished.connect(lambda out: QMessageBox.information(self, "Success", f"Created: {out}"))
        self.thread.error.connect(lambda e: QMessageBox.critical(self, "Error", e))
        self.thread.start()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    win = MergeWavApp()
    win.resize(600, 400)
    win.show()
    sys.exit(app.exec())
