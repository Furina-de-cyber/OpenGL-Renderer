import os
import numpy as np
from PIL import Image
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

class ORMTextureCreator:
    def __init__(self, root):
        self.root = root
        self.root.title("ORM纹理生成器")
        self.root.geometry("500x400")
        
        # 变量
        self.roughness_path = tk.StringVar()
        self.metallic_path = tk.StringVar()
        self.output_path = tk.StringVar()
        
        self.setup_ui()
    
    def setup_ui(self):
        # 标题
        title_label = tk.Label(self.root, text="ORM纹理生成器", font=("Arial", 16, "bold"))
        title_label.pack(pady=10)
        
        # 粗糙度图片选择
        frame1 = tk.Frame(self.root)
        frame1.pack(pady=10, padx=20, fill="x")
        
        tk.Label(frame1, text="粗糙度图片:").pack(side="left")
        tk.Entry(frame1, textvariable=self.roughness_path, width=40).pack(side="left", padx=10)
        tk.Button(frame1, text="浏览", command=lambda: self.browse_file(self.roughness_path)).pack(side="left")
        
        # 金属度图片选择
        frame2 = tk.Frame(self.root)
        frame2.pack(pady=10, padx=20, fill="x")
        
        tk.Label(frame2, text="金属度图片:").pack(side="left")
        tk.Entry(frame2, textvariable=self.metallic_path, width=40).pack(side="left", padx=10)
        tk.Button(frame2, text="浏览", command=lambda: self.browse_file(self.metallic_path)).pack(side="left")
        
        # 输出路径选择
        frame3 = tk.Frame(self.root)
        frame3.pack(pady=10, padx=20, fill="x")
        
        tk.Label(frame3, text="输出路径:").pack(side="left")
        tk.Entry(frame3, textvariable=self.output_path, width=40).pack(side="left", padx=10)
        tk.Button(frame3, text="浏览", command=self.browse_output).pack(side="left")
        
        # 进度条
        self.progress = ttk.Progressbar(self.root, mode='indeterminate')
        self.progress.pack(pady=20, padx=20, fill="x")
        
        # 生成按钮
        tk.Button(self.root, text="生成ORM纹理", command=self.create_orm, 
                 bg="green", fg="white", font=("Arial", 12), padx=20, pady=10).pack(pady=20)
        
        # 信息显示
        self.info_text = tk.Text(self.root, height=8, width=60)
        self.info_text.pack(pady=10, padx=20)
    
    def browse_file(self, path_var):
        filename = filedialog.askopenfilename(
            title="选择图片文件",
            filetypes=[("图片文件", "*.png *.jpg *.jpeg *.bmp *.tga"), ("所有文件", "*.*")]
        )
        if filename:
            path_var.set(filename)
    
    def browse_output(self):
        filename = filedialog.asksaveasfilename(
            title="保存ORM纹理",
            defaultextension=".png",
            filetypes=[("PNG图片", "*.png"), ("所有文件", "*.*")]
        )
        if filename:
            self.output_path.set(filename)
    
    def log_info(self, message):
        self.info_text.insert(tk.END, message + "\n")
        self.info_text.see(tk.END)
        self.root.update()
    
    def create_orm(self):
        roughness_path = self.roughness_path.get()
        metallic_path = self.metallic_path.get()
        output_path = self.output_path.get()
        
        if not roughness_path or not metallic_path or not output_path:
            messagebox.showerror("错误", "请填写所有路径!")
            return
        
        # 清空信息框
        self.info_text.delete(1.0, tk.END)
        
        # 开始处理
        self.progress.start()
        self.log_info("开始创建ORM纹理...")
        
        try:
            success = self.process_images(roughness_path, metallic_path, output_path)
            
            if success:
                self.log_info("\n✅ ORM纹理创建成功!")
                self.log_info(f"文件已保存到: {output_path}")
                messagebox.showinfo("成功", "ORM纹理创建成功!")
            else:
                messagebox.showerror("错误", "ORM纹理创建失败!")
        
        except Exception as e:
            self.log_info(f"\n❌ 错误: {str(e)}")
            messagebox.showerror("错误", f"处理过程中出现错误:\n{str(e)}")
        
        finally:
            self.progress.stop()
    
    def process_images(self, roughness_path, metallic_path, output_path):
        """处理图片的核心函数"""
        
        # 打开输入图片
        roughness_img = Image.open(roughness_path)
        metallic_img = Image.open(metallic_path)
        
        self.log_info(f"粗糙度图片: {os.path.basename(roughness_path)}, 模式: {roughness_img.mode}, 尺寸: {roughness_img.size}")
        self.log_info(f"金属度图片: {os.path.basename(metallic_path)}, 模式: {metallic_img.mode}, 尺寸: {metallic_img.size}")
        
        # 确保两张图片尺寸相同
        if roughness_img.size != metallic_img.size:
            self.log_info("错误: 两张图片尺寸不一致!")
            return False
        
        # 转换为RGBA模式
        if roughness_img.mode != 'RGBA':
            roughness_img = roughness_img.convert('RGBA')
        if metallic_img.mode != 'RGBA':
            metallic_img = metallic_img.convert('RGBA')
        
        # 获取图片尺寸
        width, height = roughness_img.size
        
        # 转换为numpy数组
        roughness_array = np.array(roughness_img).astype(np.float32) / 255.0
        metallic_array = np.array(metallic_img).astype(np.float32) / 255.0
        
        # 处理单通道或三通道图片
        def extract_channel(arr):
            if len(arr.shape) == 2:  # 单通道
                return arr
            else:  # 多通道
                if arr.shape[2] >= 3:
                    # 检查各通道是否相同
                    channels_same = np.allclose(arr[:,:,0], arr[:,:,1]) and np.allclose(arr[:,:,0], arr[:,:,2])
                    if channels_same:
                        self.log_info("  检测到三通道相同，使用R通道")
                        return arr[:,:,0]
                    else:
                        self.log_info("  三通道不相同，取RGB平均值")
                        return np.mean(arr[:,:,:3], axis=2)
                else:
                    return arr[:,:,0]
        
        roughness_value = extract_channel(roughness_array)
        metallic_value = extract_channel(metallic_array)
        
        self.log_info("正在创建ORM纹理...")
        
        # 创建ORM纹理
        orm_array = np.zeros((height, width, 4), dtype=np.float32)
        
        # R通道设为0（环境光遮蔽）
        orm_array[:,:,0] = 0.0
        
        # G通道设为粗糙度
        orm_array[:,:,1] = roughness_value
        
        # B通道设为金属度
        orm_array[:,:,2] = metallic_value
        
        # A通道设为1.0
        orm_array[:,:,3] = 1.0
        
        # 转换回0-255范围
        orm_array = (orm_array * 255.0).astype(np.uint8)
        
        # 保存图片
        output_img = Image.fromarray(orm_array, 'RGBA')
        output_img.save(output_path, format='PNG')
        
        self.log_info(f"ORM纹理尺寸: {width}x{height}")
        self.log_info("通道布局:")
        self.log_info("  R通道: 环境光遮蔽 (设为0)")
        self.log_info("  G通道: 粗糙度")
        self.log_info("  B通道: 金属度")
        self.log_info("  A通道: Alpha (设为1.0)")
        
        return True

# 使用示例
if __name__ == "__main__":
    # 如果您想要命令行版本，使用下面这行
    # main()  # 取消注释这行使用命令行版本
    
    # GUI版本
    root = tk.Tk()
    app = ORMTextureCreator(root)
    root.mainloop()