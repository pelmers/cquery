;;; lsp-cquery.el --- cquery client for lsp-mode     -*- lexical-binding: t; -*-

;; Copyright (C) 2017 Tobias Pisani

;; Author:  Tobias Pisani
;; Keywords: languages, lsp-mode, c++

;; Permission is hereby granted, free of charge, to any person obtaining a copy
;; of this software and associated documentation files (the "Software"), to deal
;; in the Software without restriction, including without limitation the rights
;; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;; copies of the Software, and to permit persons to whom the Software is
;; furnished to do so, subject to the following conditions:

;; The above copyright notice and this permission notice shall be included in
;; all copies or substantial portions of the Software.

;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;; SOFTWARE.
;;
;;; Commentary:

;;
;; To enable, call (lsp-cquery-enable) in your c++-mode hook.
;;
;;  TODO:
;;
;;  - Rainbow variables with semantic highlighting
;;  - Better config options
;;
;;  Style:
;;
;;  - Internal functions and variables are prefixed cquery//
;;  - Interactive functions are prefixed cquery-
;;  - All other functions and variables are prefixed cquery/
;;

;;; Code:

(require 'cc-mode)
(require 'lsp-mode)
(require 'cl-lib)
(require 'subr-x)

;; ---------------------------------------------------------------------
;;   Customization
;; ---------------------------------------------------------------------

(defgroup cquery nil
  "Customization options for the cquery client")

(defcustom cquery/root-dir
  nil
  "Your local cquery root directory. Must be set"
  :type 'directory
  :group 'cquery)

(defface cquery/inactive-region-face
  '((t :foreground "#666666"))
  "The face used to mark inactive regions"
  :group 'cquery)

(defface cquery/sem-type-face
  '((t :weight bold :inherit font-lock-type-face))
  "The face used to mark types"
  :group 'cquery)

(defface cquery/sem-member-func-face
  '((t :slant italic :inherit font-lock-function-name-face))
  "The face used to mark member functions"
  :group 'cquery)

(defface cquery/sem-free-func-face
  '((t :inherit font-lock-function-name-face))
  "The face used to mark free functions"
  :group 'cquery)

(defface cquery/sem-member-var-face
  '((t :slant italic :inherit font-lock-variable-name-face))
  "The face used to mark member variables"
  :group 'cquery)

(defface cquery/sem-free-var-face
  '((t :inherit font-lock-variable-name-face))
  "The face used to mark local and namespace scope variables"
  :group 'cquery)

(defface cquery/code-lens-face
  '((t :foreground "#777777"))
  "The face used for code lens overlays"
  :group 'cquery)

(defface cquery/code-lens-mouse-face
  '((t :box t))
  "The face used for code lens overlays"
  :group 'cquery)

(defcustom cquery/enable-sem-highlight
  t
  "Enable semantic highlighting"
  :type 'boolean
  :group 'cquery)

(defcustom cquery/sem-highlight-method
  'overlay
  "The method used to draw semantic highlighting. overlays are more
 accurate than font-lock, but slower."
  :group 'lsp-mode
  :type 'symbol
  :options '(overlay font-lock))

(defcustom cquery/cache-dir
  ".vscode/cquery_cached_index"
  "Directory in which cquery will store its index cache. Relative
 to the project root directory."
  :type 'string
  :group 'cquery)

;; ---------------------------------------------------------------------
;;   Semantic highlighting
;; ---------------------------------------------------------------------

(defun cquery//clear-sem-highlights (&optional type)
  (pcase cquery/sem-highlight-method
    ('overlay
     (dolist (ov (overlays-in 0 (point-max)))
       (when (overlay-get ov 'cquery-sem-highlight)
         (when (or (null type)
                   (eq (overlay-get ov 'cquery-sem-type) type))
           (delete-overlay ov)))))
    ('font-lock
     (font-lock-ensure))))

(defun cquery//make-sem-highlight (region buffer face)
  (pcase cquery/sem-highlight-method
    ('overlay
     (let ((ov (make-overlay (car region) (cdr region) buffer)))
       (overlay-put ov 'face face)
       (overlay-put ov 'cquery-sem-highlight t)
       (overlay-put ov 'cquery-sem-type 'highlight)))
    ('font-lock
     (put-text-property (car region) (cdr region) 'font-lock-face face buffer))))

(defun cquery//set-inactive-regions (_workspace params)
  (save-excursion
    (let* ((file (cquery//uri-to-file (gethash "uri" params)))
           (regions (mapcar 'cquery//read-range (gethash "inactiveRegions" params)))
           (buffer (find-file file)))
      (switch-to-buffer buffer)
      (cquery//clear-sem-highlights 'inactive)
      (dolist (region regions)
        (let ((ov (make-overlay (car region) (cdr region) buffer)))
          (overlay-put ov 'face 'cquery/inactive-region-face)
          (overlay-put ov 'cquery-sem-highlight t)
          (overlay-put ov 'cquery-sem-type 'inactive))))))

(defun cquery//publish-semantic-highlighting (_workspace params)
  (when cquery/enable-sem-highlight
    (save-excursion
      (let* ((file (cquery//uri-to-file (gethash "uri" params)))
             (buffer (find-file file))
             (symbols (gethash "symbols" params)))
        (switch-to-buffer buffer)
        (cquery//clear-sem-highlights 'highlight)
        (dolist (symbol symbols)
          (let* ((type (gethash "type" symbol))
                 (is-type-member (gethash "is_type_member" symbol))
                 (ranges (mapcar 'cquery//read-range (gethash "ranges" symbol)))
                 (face
                  (pcase type
                    ('0 'cquery/sem-type-face)
                    ('1 (if is-type-member 'cquery/sem-member-func-face 'cquery/sem-free-func-face))
                    ('2 (if is-type-member 'cquery/sem-member-var-face 'cquery/sem-free-var-face)))))
            (when face
              (dolist (range ranges)
                (cquery//make-sem-highlight range buffer face)))))))))

;; ---------------------------------------------------------------------
;;   Notification handlers
;; ---------------------------------------------------------------------

(defconst cquery//handlers
  '(("$cquery/setInactiveRegions" . (lambda (w p) (cquery//set-inactive-regions w p)))
    ("$cquery/publishSemanticHighlighting" . (lambda (w p) (cquery//publish-semantic-highlighting w p)))
    ("$cquery/progress" . (lambda (_w _p)))))

;; ---------------------------------------------------------------------
;;   Codelens
;;
;;   Enable by calling `cquery-request-code-lens'
;;   Clear them away using `cquery-clear-code-lens'
;;
;;   TODO:
;;   - Find a better way to display them.
;;
;;   - Instead of adding multiple lenses to one symbol, append the text
;;     of the new one to the old. This will fix flickering when moving
;;     over lenses.
;;
;;   - Add per-buffer toggle command calling request/clear functions
;;
;;   - Add a global option to request code lenses on automatically
;; ---------------------------------------------------------------------

(defun cquery-request-code-lens ()
  "Request code lens from cquery"
  (interactive)
  (lsp--cur-workspace-check)
  (lsp--send-request-async
   (lsp--make-request "textDocument/codeLens"
                      `(:textDocument (:uri ,(concat "file://" buffer-file-name))))
   'cquery//code-lens-callback))

(defun cquery-clear-code-lens ()
  "Clear all code lenses from this buffer"
  (dolist (ov (overlays-in 0 (point-max)))
    (when (overlay-get ov 'cquery-code-lens)
      (delete-overlay ov))))

(defun cquery//make-code-lens-string (command)
  (let ((map (make-sparse-keymap)))
    (define-key map [mouse-1] (lambda () (interactive) (cquery/execute-command command)))
    (propertize (gethash "title" command)
                'face 'cquery/code-lens-face
                'mouse-face 'cquery/code-lens-mouse-face
                'local-map map)))

(defun cquery//code-lens-callback (result)
  (save-excursion
    (cquery//clear-code-lens)
    (goto-char 0)
    (dolist (lens result)
      (let* ((range (cquery//read-range (gethash "range" lens)))
             (root (gethash "command" lens))
             (title (gethash "title" root))
             (command (gethash "command" root)))
        (let ((ov (make-overlay (car range) (cdr range))))
          (overlay-put ov 'cquery-code-lens t)
          (overlay-put ov 'after-string (format " %s " (cquery//make-code-lens-string root))))))))

;; ---------------------------------------------------------------------
;;   CodeAction Commands
;; ---------------------------------------------------------------------

(defun cquery-select-codeaction ()
  "Show a list of codeactions using ivy, and pick one to apply"
  (interactive)
  (let ((name-func
         (lambda (action)
           (let ((edit (caadr (gethash "arguments" action))))
             (format "%s: %s" (cquery//pos-at-hashed-pos
                               (gethash "start" (gethash "range" edit)))
                     (gethash "title" action))))))
    (if (null lsp-code-actions)
        (message "No code actions avaliable")
      (ivy-read "Apply CodeAction: "
                (mapcar (lambda (action)
                          (funcall name-func action))
                        lsp-code-actions)
                :action (lambda (str)
                          (dolist (action lsp-code-actions)
                            (when (equal (funcall name-func action) str)
                              (cquery/execute-command action)
                              (lsp--text-document-code-action))))))))

(defun cquery/execute-command (action)
  "Execute a cquery command"
  (let* ((command (gethash "command" action))
         (arguments (gethash "arguments" action))
         (uri (car arguments))
         (data (cdr arguments)))
    (switch-to-buffer (find-file (cquery//uri-to-file uri)))
    (pcase command
      ;; Code actions
      ('"cquery._applyFixIt"
       (dolist (edit data)
         (cquery//apply-textedit (car edit))))
      ('"cquery._autoImplement"
       (dolist (edit data)
         (cquery//apply-textedit (car edit)))
       (goto-char (cquery//pos-at-hashed-pos
                   (gethash "start" (gethash "range" (caar data))))))
      ('"cquery._insertInclude"
       (cquery//select-textedit data "Include: "))
      ('"cquery.showReferences" ;; Used by code lenses
       (let ((pos (cquery//pos-at-hashed-pos (car data))))
         (goto-char pos)
         (xref-find-references (xref-backend-identifier-at-point (xref-find-backend))))))))

(defun cquery//select-textedit (edit-list prompt)
  "Show a list of possible textedits, and apply the selected.
  Used by cquery._insertInclude"
  (let ((name-func (lambda (edit)
                     (concat (cquery//pos-at-hashed-pos
                              (gethash "start" (gethash "range" edit)))
                             ": "
                             (gethash "newText" edit)))))
    (ivy-read prompt
              (mapcar (lambda (edit)
                        (funcall name-func edit))
                      edit-list)
              :require-match t
              :action (lambda (str)
                        (cl-loop
                         for edit in edit-list
                         do (when (equal (funcall name-func edit) str)
                              (cquery//apply-textedit edit)))))))

(defun cquery//apply-textedit (edit)
  (let* ((range (gethash "range" edit))
         (start (cquery//pos-at-hashed-pos (gethash "start" range)))
         (end (cquery//pos-at-hashed-pos (gethash "end" range)))
         (newText (gethash "newText" edit)))
    (when (> end start)
      (delete-region start (- end 1)))
    (goto-char start)
    (insert newText)))

(defun cquery//uri-to-file (uri)
  (string-remove-prefix "file://" uri))

(defun cquery//read-range (range)
  `(,(cquery//pos-at-hashed-pos (gethash "start" range)) . ,(cquery//pos-at-hashed-pos (gethash "end" range))))

(defun cquery//pos-at-hashed-pos (hashed)
  (cquery//pos-at-line-col
   (gethash "line" hashed)
   (gethash "character" hashed)))

(defun cquery//pos-at-line-col (l c)
  (save-excursion
    (goto-char (point-min))
    (forward-line l)
    (move-to-column c)
    (point)))

;; ---------------------------------------------------------------------
;;  Register lsp client
;; ---------------------------------------------------------------------

(defun cquery//render-string (str)
  (condition-case nil
      (with-temp-buffer
	    (delay-mode-hooks (c++-mode))
	    (insert str)
	    (font-lock-ensure)
	    (buffer-string))
    (error str)))

(defun cquery//initialize-client (client)
  (dolist (p cquery//handlers)
    (lsp-client-on-notification client (car p) (cdr p)))
  (lsp-provide-marked-string-renderer client "c++" #'cquery//render-string))

(defun cquery//get-init-params (workspace)
  `(:cacheDirectory ,(concat (lsp--workspace-root workspace) cquery/cache-dir)
     :resourceDirectory ,(concat cquery/root-dir "clang_resource_dir")))

(defun cquery//get-root ()
  "Return the root directory of a cquery project."
  (when (null cquery/root-dir)
    (user-error "Set cquery/root-dir to the path of your cquery directory using customize"))
  (expand-file-name (or (locate-dominating-file default-directory "compile_commands.json")
                        (locate-dominating-file default-directory "clang_args")
                        (user-error "Could not find cquery project root"))))

(lsp-define-stdio-client
 lsp-cquery "c++" #'cquery//get-root
 (list (concat cquery/root-dir "build/app") "--language-server")
 :initialize #'cquery//initialize-client
 :extra-init-params #'cquery//get-init-params)

;; ---------------------------------------------------------------------
;;  lsp-mode function advices
;; ---------------------------------------------------------------------

;; For some reason this function just adds code actions in lsp-mode. We need to
;; clear the old ones
(advice-add 'lsp--text-document-code-action-callback :around
            '(lambda (orig actions) (setq lsp-code-actions actions)))

(provide 'lsp-cquery)
;;; lsp-cquery.el ends here
