namespace GuiLoaderCS
{
    partial class frmMain
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.label1 = new System.Windows.Forms.Label();
            this.tbDllPath = new System.Windows.Forms.TextBox();
            this.button1 = new System.Windows.Forms.Button();
            this.label2 = new System.Windows.Forms.Label();
            this.tbProcessName = new System.Windows.Forms.TextBox();
            this.button2 = new System.Windows.Forms.Button();
            this.tbLog = new System.Windows.Forms.TextBox();
            this.gbInjectionMethod = new System.Windows.Forms.GroupBox();
            this.rbWindowHook = new System.Windows.Forms.RadioButton();
            this.rbManualMap = new System.Windows.Forms.RadioButton();
            this.gbInjectionMethod.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 9);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(72, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "DLL To Inject";
            // 
            // tbDllPath
            // 
            this.tbDllPath.Location = new System.Drawing.Point(129, 6);
            this.tbDllPath.Name = "tbDllPath";
            this.tbDllPath.Size = new System.Drawing.Size(228, 20);
            this.tbDllPath.TabIndex = 1;
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(363, 4);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(23, 23);
            this.button1.TabIndex = 2;
            this.button1.Text = "…";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(13, 35);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(110, 13);
            this.label2.TabIndex = 3;
            this.label2.Text = "Target Process Name";
            // 
            // tbProcessName
            // 
            this.tbProcessName.Location = new System.Drawing.Point(129, 32);
            this.tbProcessName.Name = "tbProcessName";
            this.tbProcessName.Size = new System.Drawing.Size(228, 20);
            this.tbProcessName.TabIndex = 4;
            // 
            // button2
            // 
            this.button2.Location = new System.Drawing.Point(12, 105);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(374, 23);
            this.button2.TabIndex = 5;
            this.button2.Text = "GO";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // tbLog
            // 
            this.tbLog.Location = new System.Drawing.Point(12, 134);
            this.tbLog.Multiline = true;
            this.tbLog.Name = "tbLog";
            this.tbLog.ReadOnly = true;
            this.tbLog.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.tbLog.Size = new System.Drawing.Size(374, 178);
            this.tbLog.TabIndex = 6;
            // 
            // gbInjectionMethod
            // 
            this.gbInjectionMethod.Controls.Add(this.rbManualMap);
            this.gbInjectionMethod.Controls.Add(this.rbWindowHook);
            this.gbInjectionMethod.Location = new System.Drawing.Point(12, 58);
            this.gbInjectionMethod.Name = "gbInjectionMethod";
            this.gbInjectionMethod.Size = new System.Drawing.Size(374, 41);
            this.gbInjectionMethod.TabIndex = 7;
            this.gbInjectionMethod.TabStop = false;
            this.gbInjectionMethod.Text = "Injection Method";
            // 
            // rbWindowHook
            // 
            this.rbWindowHook.AutoSize = true;
            this.rbWindowHook.Location = new System.Drawing.Point(7, 18);
            this.rbWindowHook.Name = "rbWindowHook";
            this.rbWindowHook.Size = new System.Drawing.Size(93, 17);
            this.rbWindowHook.TabIndex = 0;
            this.rbWindowHook.TabStop = true;
            this.rbWindowHook.Text = "Window Hook";
            this.rbWindowHook.UseVisualStyleBackColor = true;
            // 
            // rbManualMap
            // 
            this.rbManualMap.AutoSize = true;
            this.rbManualMap.Location = new System.Drawing.Point(106, 18);
            this.rbManualMap.Name = "rbManualMap";
            this.rbManualMap.Size = new System.Drawing.Size(84, 17);
            this.rbManualMap.TabIndex = 1;
            this.rbManualMap.TabStop = true;
            this.rbManualMap.Text = "Manual Map";
            this.rbManualMap.UseVisualStyleBackColor = true;
            // 
            // frmMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(398, 324);
            this.Controls.Add(this.gbInjectionMethod);
            this.Controls.Add(this.tbLog);
            this.Controls.Add(this.button2);
            this.Controls.Add(this.tbProcessName);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.tbDllPath);
            this.Controls.Add(this.label1);
            this.Name = "frmMain";
            this.Text = "DLL Injector";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.frmMain_FormClosing);
            this.Load += new System.EventHandler(this.frmMain_Load);
            this.gbInjectionMethod.ResumeLayout(false);
            this.gbInjectionMethod.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox tbDllPath;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.TextBox tbProcessName;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.TextBox tbLog;
        private System.Windows.Forms.GroupBox gbInjectionMethod;
        private System.Windows.Forms.RadioButton rbManualMap;
        private System.Windows.Forms.RadioButton rbWindowHook;
    }
}

