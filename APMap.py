import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib import cm
import matplotlib.patches as patches
from matplotlib.patches import Rectangle, Circle
import random
from collections import defaultdict
import json
import pandas as pd
from typing import List, Tuple, Dict, Any
from dataclasses import dataclass
import heapq
from enum import Enum

# 常量定义
FLOOR_HEIGHT = 3.5  # 每层高度
BUILDING_X = 100   # 建筑X方向尺寸(米)
BUILDING_Y = 80    # 建筑Y方向尺寸(米)
BUILDING_Z = 3     # 建筑层数
GRID_SIZE = 1.0    # 网格大小(米)
SIGNAL_THRESHOLD = -65.0  # 信号强度阈值(dBm)
AP_TX_POWER = 20.0  # AP发射功率(dBm)
FREQ_2_4 = 2400.0  # 2.4GHz频率(MHz)
FREQ_5 = 5200.0    # 5GHz频率(MHz)

# 墙体类型
class WallType(Enum):
    AIR = 0         # 空气
    CONCRETE = 1    # 承重墙(钢筋混凝土)
    PLASTER = 2     # 普通隔断墙(石膏板/木板)
    GLASS = 3       # 玻璃幕墙
    DOOR = 4        # 门(衰减较小)
    
# 墙体衰减值(dB)
WALL_ATTENUATION = {
    WallType.AIR: 0.0,
    WallType.CONCRETE: 12.0,
    WallType.PLASTER: 6.0,
    WallType.GLASS: 7.0,
    WallType.DOOR: 3.0
}

# 墙体厚度(米)
WALL_THICKNESS = {
    WallType.AIR: 0.0,
    WallType.CONCRETE: 0.25,
    WallType.PLASTER: 0.1,
    WallType.GLASS: 0.05,
    WallType.DOOR: 0.05
}

@dataclass
class GridPoint:
    """网格点"""
    x: float
    y: float
    z: float
    wall_type: WallType
    signal_2_4: float = -100.0
    signal_5: float = -100.0
    best_ap_id: int = -1
    covered_2_4: bool = False
    covered_5: bool = False
    in_corridor: bool = False
    in_room: bool = False
    room_id: int = -1

@dataclass
class AccessPoint:
    """无线接入点"""
    id: int
    x: float
    y: float
    z: float
    floor: int
    channel_2_4: int = 1
    channel_5: int = 36
    coverage_radius_2_4: float = 20.0
    coverage_radius_5: float = 12.0
    tx_power: float = 20.0
    is_active: bool = True
    
@dataclass
class Room:
    """教室/房间"""
    id: int
    x: float
    y: float
    width: float
    height: float
    floor: int
    name: str = ""

class Building:
    """教学楼建筑"""
    
    def __init__(self, building_x=100, building_y=80, building_z=3, floor_height=3.5, grid_size=1.0):
        self.building_x = building_x
        self.building_y = building_y
        self.building_z = building_z
        self.floor_height = floor_height
        self.grid_size = grid_size
        
        # 建筑网格
        self.grid = None
        # AP列表
        self.access_points = []
        # 房间列表
        self.rooms = []
        # 覆盖统计
        self.coverage_stats = {}
        
        # 初始化建筑
        self.initialize_building()
    
    def initialize_building(self):
        """初始化建筑结构"""
        # 创建三维网格
        self.grid = np.empty(
            (int(self.building_x / self.grid_size), 
             int(self.building_y / self.grid_size), 
             self.building_z), 
            dtype=object
        )
        
        # 初始化网格点
        for z in range(self.building_z):
            for y_idx in range(self.grid.shape[1]):
                for x_idx in range(self.grid.shape[0]):
                    x = x_idx * self.grid_size
                    y = y_idx * self.grid_size
                    z_pos = (z + 0.5) * self.floor_height
                    
                    # 默认设置为空气
                    wall_type = WallType.AIR
                    
                    # 检查是否在外墙
                    if (x_idx == 0 or x_idx == self.grid.shape[0] - 1 or 
                        y_idx == 0 or y_idx == self.grid.shape[1] - 1):
                        wall_type = WallType.CONCRETE
                    
                    # 楼层之间的地板/天花板
                    if z > 0:  # 不是第一层
                        wall_type = WallType.CONCRETE
                    
                    # 创建网格点
                    grid_point = GridPoint(x, y, z_pos, wall_type)
                    
                    # 走廊检查
                    corridor_center_y = self.building_y / 2
                    corridor_width = 2.0
                    if (corridor_center_y - corridor_width/2 <= y <= corridor_center_y + corridor_width/2):
                        grid_point.in_corridor = True
                    
                    self.grid[x_idx, y_idx, z] = grid_point
        
        # 创建走廊
        self.create_corridor()
        
        # 创建教室
        self.create_classrooms()
        
        # 设置玻璃幕墙
        self.create_glass_walls()
        
        print(f"建筑初始化完成: {self.building_x}m x {self.building_y}m x {self.building_z}层")
    
    def create_corridor(self):
        """创建走廊"""
        corridor_center_y = self.building_y / 2
        corridor_width = 2.0
        
        for z in range(self.building_z):
            for y_idx in range(self.grid.shape[1]):
                for x_idx in range(self.grid.shape[0]):
                    y = y_idx * self.grid_size
                    if (corridor_center_y - corridor_width/2 <= y <= corridor_center_y + corridor_width/2):
                        point = self.grid[x_idx, y_idx, z]
                        point.wall_type = WallType.AIR
                        point.in_corridor = True
                        
                        # 走廊的门
                        if x_idx % 20 == 0:  # 每20米一个门
                            point.wall_type = WallType.DOOR
    
    def create_classrooms(self):
        """创建教室"""
        classroom_width = 15.0
        classroom_height = 10.0
        corridor_width = 2.0
        
        for z in range(self.building_z):
            room_id = 0
            
            # 左侧教室 (3个)
            for i in range(3):
                start_x = 1.0 + i * (classroom_width + 1.0)
                start_y = 1.0
                
                # 创建教室对象
                room = Room(
                    id=room_id,
                    x=start_x,
                    y=start_y,
                    width=classroom_width,
                    height=classroom_height,
                    floor=z,
                    name=f"教室 {z+1}-{i+1}"
                )
                self.rooms.append(room)
                
                # 标记教室内的网格点
                for x_idx in range(self.grid.shape[0]):
                    for y_idx in range(self.grid.shape[1]):
                        x = x_idx * self.grid_size
                        y = y_idx * self.grid_size
                        
                        if (start_x <= x <= start_x + classroom_width and 
                            start_y <= y <= start_y + classroom_height):
                            point = self.grid[x_idx, y_idx, z]
                            point.in_room = True
                            point.room_id = room_id
                            
                            # 如果是墙壁位置
                            if (x <= start_x + 0.5 or x >= start_x + classroom_width - 0.5 or
                                y <= start_y + 0.5 or y >= start_y + classroom_height - 0.5):
                                point.wall_type = WallType.PLASTER
                            
                            # 教室门 (在走廊侧)
                            if (abs(y - (start_y + classroom_height)) < 0.5 and 
                                abs(x - (start_x + classroom_width/2)) < 1.0):
                                point.wall_type = WallType.DOOR
                
                room_id += 1
            
            # 右侧教室 (3个)
            for i in range(3):
                start_x = 51.0 + i * (classroom_width + 1.0)
                start_y = 1.0
                
                room = Room(
                    id=room_id,
                    x=start_x,
                    y=start_y,
                    width=classroom_width,
                    height=classroom_height,
                    floor=z,
                    name=f"教室 {z+1}-{i+4}"
                )
                self.rooms.append(room)
                
                for x_idx in range(self.grid.shape[0]):
                    for y_idx in range(self.grid.shape[1]):
                        x = x_idx * self.grid_size
                        y = y_idx * self.grid_size
                        
                        if (start_x <= x <= start_x + classroom_width and 
                            start_y <= y <= start_y + classroom_height):
                            point = self.grid[x_idx, y_idx, z]
                            point.in_room = True
                            point.room_id = room_id
                            
                            if (x <= start_x + 0.5 or x >= start_x + classroom_width - 0.5 or
                                y <= start_y + 0.5 or y >= start_y + classroom_height - 0.5):
                                point.wall_type = WallType.PLASTER
                            
                            if (abs(y - (start_y + classroom_height)) < 0.5 and 
                                abs(x - (start_x + classroom_width/2)) < 1.0):
                                point.wall_type = WallType.DOOR
                
                room_id += 1
    
    def create_glass_walls(self):
        """创建玻璃幕墙"""
        corridor_center_y = self.building_y / 2
        
        for z in range(self.building_z):
            for x_idx in range(self.grid.shape[0]):
                # 走廊两侧的玻璃幕墙
                y_left = int((corridor_center_y - 1.0) / self.grid_size)
                y_right = int((corridor_center_y + 1.0) / self.grid_size)
                
                if 0 <= y_left < self.grid.shape[1]:
                    point = self.grid[x_idx, y_left, z]
                    if point.wall_type == WallType.AIR:
                        point.wall_type = WallType.GLASS
                
                if 0 <= y_right < self.grid.shape[1]:
                    point = self.grid[x_idx, y_right, z]
                    if point.wall_type == WallType.AIR:
                        point.wall_type = WallType.GLASS
    
    def calculate_distance(self, x1, y1, z1, x2, y2, z2):
        """计算两点间距离"""
        return np.sqrt((x2 - x1)**2 + (y2 - y1)**2 + (z2 - z1)**2)
    
    def calculate_wall_attenuation(self, x1, y1, z1, x2, y2, z2):
        """计算路径上的墙体衰减"""
        # 简化模型：直线路径上的墙体衰减
        # 使用Bresenham直线算法近似计算穿过的墙体
        total_attenuation = 0.0
        
        # 将坐标转换为网格索引
        x1_idx = int(x1 / self.grid_size)
        y1_idx = int(y1 / self.grid_size)
        z1_idx = int(z1 / self.floor_height)
        
        x2_idx = int(x2 / self.grid_size)
        y2_idx = int(y2 / self.grid_size)
        z2_idx = int(z2 / self.floor_height)
        
        # 确保索引在范围内
        x1_idx = max(0, min(x1_idx, self.grid.shape[0]-1))
        y1_idx = max(0, min(y1_idx, self.grid.shape[1]-1))
        z1_idx = max(0, min(z1_idx, self.grid.shape[2]-1))
        
        x2_idx = max(0, min(x2_idx, self.grid.shape[0]-1))
        y2_idx = max(0, min(y2_idx, self.grid.shape[1]-1))
        z2_idx = max(0, min(z2_idx, self.grid.shape[2]-1))
        
        # 如果起点和终点在同一网格，简化处理
        if x1_idx == x2_idx and y1_idx == y2_idx and z1_idx == z2_idx:
            return 0.0
        
        # 计算直线路径
        dx = abs(x2_idx - x1_idx)
        dy = abs(y2_idx - y1_idx)
        dz = abs(z2_idx - z1_idx)
        
        steps = max(dx, dy, dz)
        if steps == 0:
            return 0.0
        
        x_inc = (x2_idx - x1_idx) / steps
        y_inc = (y2_idx - y1_idx) / steps
        z_inc = (z2_idx - z1_idx) / steps
        
        x = x1_idx
        y = y1_idx
        z = z1_idx
        
        visited = set()
        
        for _ in range(steps + 1):
            xi = int(round(x))
            yi = int(round(y))
            zi = int(round(z))
            
            if 0 <= xi < self.grid.shape[0] and 0 <= yi < self.grid.shape[1] and 0 <= zi < self.grid.shape[2]:
                key = (xi, yi, zi)
                if key not in visited:
                    visited.add(key)
                    grid_point = self.grid[xi, yi, zi]
                    if grid_point and grid_point.wall_type != WallType.AIR:
                        total_attenuation += WALL_ATTENUATION[grid_point.wall_type]
            
            x += x_inc
            y += y_inc
            z += z_inc
        
        return total_attenuation
    
    def calculate_path_loss(self, distance, frequency, wall_attenuation):
        """计算路径损耗"""
        if distance < 1.0:
            distance = 1.0
        
        # 自由空间路径损耗公式
        # PL(dB) = 20*log10(d) + 20*log10(f) - 147.55
        # 其中d为距离(米)，f为频率(MHz)
        pl = 20.0 * np.log10(distance) + 20.0 * np.log10(frequency) - 147.55
        
        # 加上墙体衰减
        pl += wall_attenuation
        
        return pl
    
    def calculate_signal_strength(self, ap, grid_point, frequency):
        """计算信号强度"""
        # 计算距离
        distance = self.calculate_distance(
            ap.x, ap.y, ap.z,
            grid_point.x, grid_point.y, grid_point.z
        )
        
        # 如果距离太远，直接返回很弱的信号
        if distance > 100.0:  # 最大考虑100米
            return -100.0
        
        # 计算墙体衰减
        wall_atten = self.calculate_wall_attenuation(
            ap.x, ap.y, ap.z,
            grid_point.x, grid_point.y, grid_point.z
        )
        
        # 计算路径损耗
        path_loss = self.calculate_path_loss(distance, frequency, wall_atten)
        
        # 计算接收信号强度
        rssi = ap.tx_power - path_loss
        
        return rssi
    
    def deploy_aps_strategic(self, aps_per_floor=5):
        """策略性部署AP"""
        self.access_points = []
        ap_id = 0
        
        for z in range(self.building_z):
            floor_z = (z + 0.5) * self.floor_height
            corridor_center_y = self.building_y / 2
            
            # 在走廊中心线等间距部署
            spacing = self.building_x / (aps_per_floor + 1)
            
            for i in range(aps_per_floor):
                ap_x = (i + 1) * spacing
                ap_y = corridor_center_y
                
                # 创建AP
                ap = AccessPoint(
                    id=ap_id,
                    x=ap_x,
                    y=ap_y,
                    z=floor_z,
                    floor=z+1
                )
                
                self.access_points.append(ap)
                ap_id += 1
        
        print(f"已部署 {len(self.access_points)} 个AP")
        
        # 分配信道
        self.assign_channels()
        
        return self.access_points
    
    def assign_channels(self):
        """分配信道以避免干扰"""
        # 2.4GHz非重叠信道: 1, 6, 11
        channels_2_4 = [1, 6, 11]
        # 5GHz信道
        channels_5 = [36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144]
        
        for i, ap in enumerate(self.access_points):
            # 2.4GHz信道：同一楼层的AP使用不同信道
            ap.channel_2_4 = channels_2_4[(i + ap.floor) % len(channels_2_4)]
            
            # 5GHz信道：尽量使用不同信道
            ap.channel_5 = channels_5[i % len(channels_5)]
    
    def calculate_coverage(self):
        """计算覆盖范围"""
        print("计算信号覆盖...")
        
        # 重置所有网格点的信号
        for z in range(self.grid.shape[2]):
            for y in range(self.grid.shape[1]):
                for x in range(self.grid.shape[0]):
                    grid_point = self.grid[x, y, z]
                    if grid_point:
                        grid_point.signal_2_4 = -100.0
                        grid_point.signal_5 = -100.0
                        grid_point.best_ap_id = -1
                        grid_point.covered_2_4 = False
                        grid_point.covered_5 = False
        
        # 对每个AP计算信号强度
        for ap in self.access_points:
            if not ap.is_active:
                continue
                
            for z in range(self.grid.shape[2]):
                for y in range(self.grid.shape[1]):
                    for x in range(self.grid.shape[0]):
                        grid_point = self.grid[x, y, z]
                        if not grid_point:
                            continue
                        
                        # 计算距离
                        distance = self.calculate_distance(
                            ap.x, ap.y, ap.z,
                            grid_point.x, grid_point.y, grid_point.z
                        )
                        
                        # 如果距离太远，跳过
                        if distance > 50.0:  # 最大覆盖距离
                            continue
                        
                        # 计算2.4GHz信号
                        signal_2_4 = self.calculate_signal_strength(ap, grid_point, FREQ_2_4)
                        if signal_2_4 > grid_point.signal_2_4:
                            grid_point.signal_2_4 = signal_2_4
                            if signal_2_4 >= SIGNAL_THRESHOLD:
                                grid_point.covered_2_4 = True
                                if signal_2_4 > grid_point.signal_5:  # 暂时比较
                                    grid_point.best_ap_id = ap.id
                        
                        # 计算5GHz信号
                        signal_5 = self.calculate_signal_strength(ap, grid_point, FREQ_5)
                        if signal_5 > grid_point.signal_5:
                            grid_point.signal_5 = signal_5
                            if signal_5 >= SIGNAL_THRESHOLD:
                                grid_point.covered_5 = True
        
        # 计算覆盖统计
        self.calculate_coverage_statistics()
    
    def calculate_coverage_statistics(self):
        """计算覆盖统计"""
        total_points = 0
        covered_2_4_points = 0
        covered_5_points = 0
        covered_both_points = 0
        
        for z in range(self.grid.shape[2]):
            for y in range(self.grid.shape[1]):
                for x in range(self.grid.shape[0]):
                    grid_point = self.grid[x, y, z]
                    if grid_point:
                        total_points += 1
                        if grid_point.covered_2_4:
                            covered_2_4_points += 1
                        if grid_point.covered_5:
                            covered_5_points += 1
                        if grid_point.covered_2_4 and grid_point.covered_5:
                            covered_both_points += 1
        
        self.coverage_stats = {
            'total_points': total_points,
            'covered_2_4': covered_2_4_points,
            'coverage_rate_2_4': (covered_2_4_points / total_points * 100) if total_points > 0 else 0,
            'covered_5': covered_5_points,
            'coverage_rate_5': (covered_5_points / total_points * 100) if total_points > 0 else 0,
            'covered_both': covered_both_points,
            'coverage_rate_both': (covered_both_points / total_points * 100) if total_points > 0 else 0
        }
        
        return self.coverage_stats
    
    def optimize_ap_positions(self, max_iterations=10):
        """优化AP位置"""
        print("优化AP位置...")
        
        best_aps = self.access_points.copy()
        best_coverage_rate = self.coverage_stats['coverage_rate_both']
        
        for iteration in range(max_iterations):
            print(f"优化迭代 {iteration + 1}/{max_iterations}")
            
            for i, ap in enumerate(self.access_points):
                original_x, original_y = ap.x, ap.y
                
                # 在走廊范围内尝试不同位置
                corridor_center_y = self.building_y / 2
                best_x, best_y = original_x, original_y
                local_best_coverage = self.coverage_stats['coverage_rate_both']
                
                # 尝试几个候选位置
                for dx in [-5, -2, 0, 2, 5]:
                    for dy in [-2, 0, 2]:
                        new_x = original_x + dx
                        new_y = corridor_center_y + dy
                        
                        # 确保位置在建筑范围内
                        if 1 <= new_x <= self.building_x - 1 and 1 <= new_y <= self.building_y - 1:
                            # 检查是否在走廊内
                            corridor_width = 2.0
                            if abs(new_y - corridor_center_y) <= corridor_width / 2:
                                # 临时移动AP
                                ap.x = new_x
                                ap.y = new_y
                                
                                # 重新计算覆盖
                                self.calculate_coverage()
                                current_coverage = self.coverage_stats['coverage_rate_both']
                                
                                # 如果更好，更新最佳位置
                                if current_coverage > local_best_coverage:
                                    local_best_coverage = current_coverage
                                    best_x, best_y = new_x, new_y
                
                # 恢复最佳位置
                ap.x, ap.y = best_x, best_y
                
                # 重新计算覆盖
                self.calculate_coverage()
                current_coverage = self.coverage_stats['coverage_rate_both']
                
                # 更新全局最佳
                if current_coverage > best_coverage_rate:
                    best_coverage_rate = current_coverage
                    best_aps = [AccessPoint(ap.id, ap.x, ap.y, ap.z, ap.floor, 
                                          ap.channel_2_4, ap.channel_5, 
                                          ap.coverage_radius_2_4, ap.coverage_radius_5, 
                                          ap.tx_power, ap.is_active) for ap in self.access_points]
        
        # 使用最佳AP配置
        self.access_points = best_aps
        self.calculate_coverage()
        
        print(f"优化完成，最佳覆盖率为: {best_coverage_rate:.2f}%")
        
        return best_coverage_rate
    
    def visualize_heatmap(self, floor=0, save_fig=True):
        """可视化热力图"""
        if floor >= self.building_z:
            print(f"楼层 {floor} 不存在，有效楼层: 0-{self.building_z-1}")
            return
        
        fig, axes = plt.subplots(2, 3, figsize=(18, 12))
        
        # 准备数据
        x_size = self.grid.shape[0]
        y_size = self.grid.shape[1]
        
        signal_2_4 = np.zeros((y_size, x_size))
        signal_5 = np.zeros((y_size, x_size))
        coverage_2_4 = np.zeros((y_size, x_size))
        coverage_5 = np.zeros((y_size, x_size))
        best_ap = np.zeros((y_size, x_size))
        wall_map = np.zeros((y_size, x_size))
        
        for x_idx in range(x_size):
            for y_idx in range(y_size):
                grid_point = self.grid[x_idx, y_idx, floor]
                if grid_point:
                    signal_2_4[y_idx, x_idx] = grid_point.signal_2_4
                    signal_5[y_idx, x_idx] = grid_point.signal_5
                    coverage_2_4[y_idx, x_idx] = 1 if grid_point.covered_2_4 else 0
                    coverage_5[y_idx, x_idx] = 1 if grid_point.covered_5 else 0
                    best_ap[y_idx, x_idx] = grid_point.best_ap_id if grid_point.best_ap_id >= 0 else -1
                    wall_map[y_idx, x_idx] = grid_point.wall_type.value
        
        # 1. 2.4GHz信号热力图
        im1 = axes[0, 0].imshow(signal_2_4, cmap='RdYlGn_r', aspect='auto', 
                               origin='lower', vmin=-100, vmax=-30, 
                               extent=[0, self.building_x, 0, self.building_y])
        axes[0, 0].set_title(f'楼层 {floor+1} - 2.4GHz 信号强度 (dBm)')
        axes[0, 0].set_xlabel('X (m)')
        axes[0, 0].set_ylabel('Y (m)')
        plt.colorbar(im1, ax=axes[0, 0], label='信号强度 (dBm)')
        
        # 标记AP位置
        for ap in self.access_points:
            if ap.floor == floor + 1:
                axes[0, 0].scatter(ap.x, ap.y, c='red', s=100, marker='^', edgecolors='black', linewidth=1)
                axes[0, 0].text(ap.x, ap.y+2, f'AP{ap.id}', fontsize=8, ha='center')
        
        # 2. 5GHz信号热力图
        im2 = axes[0, 1].imshow(signal_5, cmap='RdYlGn_r', aspect='auto', 
                               origin='lower', vmin=-100, vmax=-30,
                               extent=[0, self.building_x, 0, self.building_y])
        axes[0, 1].set_title(f'楼层 {floor+1} - 5GHz 信号强度 (dBm)')
        axes[0, 1].set_xlabel('X (m)')
        axes[0, 1].set_ylabel('Y (m)')
        plt.colorbar(im2, ax=axes[0, 1], label='信号强度 (dBm)')
        
        for ap in self.access_points:
            if ap.floor == floor + 1:
                axes[0, 1].scatter(ap.x, ap.y, c='red', s=100, marker='^', edgecolors='black', linewidth=1)
        
        # 3. 2.4GHz覆盖图
        im3 = axes[0, 2].imshow(coverage_2_4, cmap='RdYlGn', aspect='auto', 
                               origin='lower', vmin=0, vmax=1,
                               extent=[0, self.building_x, 0, self.building_y])
        axes[0, 2].set_title(f'楼层 {floor+1} - 2.4GHz 覆盖情况')
        axes[0, 2].set_xlabel('X (m)')
        axes[0, 2].set_ylabel('Y (m)')
        
        # 4. 5GHz覆盖图
        im4 = axes[1, 0].imshow(coverage_5, cmap='RdYlGn', aspect='auto', 
                               origin='lower', vmin=0, vmax=1,
                               extent=[0, self.building_x, 0, self.building_y])
        axes[1, 0].set_title(f'楼层 {floor+1} - 5GHz 覆盖情况')
        axes[1, 0].set_xlabel('X (m)')
        axes[1, 0].set_ylabel('Y (m)')
        
        # 5. 最佳AP分布
        im5 = axes[1, 1].imshow(best_ap, cmap='tab20', aspect='auto', 
                               origin='lower', 
                               extent=[0, self.building_x, 0, self.building_y])
        axes[1, 1].set_title(f'楼层 {floor+1} - 最佳AP分布')
        axes[1, 1].set_xlabel('X (m)')
        axes[1, 1].set_ylabel('Y (m)')
        plt.colorbar(im5, ax=axes[1, 1], label='AP ID')
        
        # 6. 建筑结构图
        im6 = axes[1, 2].imshow(wall_map, cmap='Set3', aspect='auto', 
                               origin='lower', 
                               extent=[0, self.building_x, 0, self.building_y])
        axes[1, 2].set_title(f'楼层 {floor+1} - 建筑结构')
        axes[1, 2].set_xlabel('X (m)')
        axes[1, 2].set_ylabel('Y (m)')
        
        # 添加图例
        wall_types = ['空气', '承重墙', '隔断墙', '玻璃', '门']
        colors = [im6.cmap(i/4) for i in range(5)]
        from matplotlib.patches import Patch
        legend_elements = [Patch(facecolor=colors[i], label=wall_types[i]) for i in range(5)]
        axes[1, 2].legend(handles=legend_elements, loc='upper right', fontsize=8)
        
        plt.tight_layout()
        
        if save_fig:
            plt.savefig(f'floor_{floor+1}_coverage.png', dpi=300, bbox_inches='tight')
            print(f"已保存热力图: floor_{floor+1}_coverage.png")
        
        plt.show()
        
        return fig
    
    def visualize_3d_coverage(self, floor=0, save_fig=True):
        """3D可视化"""
        if floor >= self.building_z:
            print(f"楼层 {floor} 不存在")
            return
        
        # 准备数据
        x_size = self.grid.shape[0]
        y_size = self.grid.shape[1]
        
        X, Y = np.meshgrid(np.arange(x_size) * self.grid_size, 
                          np.arange(y_size) * self.grid_size)
        Z = np.zeros((y_size, x_size))
        
        for x_idx in range(x_size):
            for y_idx in range(y_size):
                grid_point = self.grid[x_idx, y_idx, floor]
                if grid_point:
                    Z[y_idx, x_idx] = max(grid_point.signal_2_4, grid_point.signal_5)
        
        # 创建3D图形
        fig = plt.figure(figsize=(14, 10))
        ax = fig.add_subplot(111, projection='3d')
        
        # 绘制3D曲面
        surf = ax.plot_surface(X, Y, Z, cmap='RdYlGn_r', 
                              linewidth=0, antialiased=True, alpha=0.8)
        
        # 标记AP位置
        for ap in self.access_points:
            if ap.floor == floor + 1:
                # 计算该AP的信号强度
                ap_signal = -30  # AP自身位置信号最强
                ax.scatter(ap.x, ap.y, ap_signal, c='red', s=100, marker='^', 
                          edgecolors='black', linewidth=1, label=f'AP{ap.id}')
        
        ax.set_title(f'楼层 {floor+1} - 3D信号覆盖')
        ax.set_xlabel('X (m)')
        ax.set_ylabel('Y (m)')
        ax.set_zlabel('信号强度 (dBm)')
        ax.set_zlim(-100, -30)
        
        fig.colorbar(surf, ax=ax, shrink=0.5, aspect=5, label='信号强度 (dBm)')
        
        if save_fig:
            plt.savefig(f'floor_{floor+1}_3d_coverage.png', dpi=300, bbox_inches='tight')
            print(f"已保存3D图: floor_{floor+1}_3d_coverage.png")
        
        plt.show()
        
        return fig
    
    def visualize_ap_distribution(self, save_fig=True):
        """可视化AP分布"""
        fig, axes = plt.subplots(1, self.building_z, figsize=(6*self.building_z, 8))
        
        if self.building_z == 1:
            axes = [axes]
        
        for floor in range(self.building_z):
            ax = axes[floor]
            
            # 绘制建筑轮廓
            ax.add_patch(Rectangle((0, 0), self.building_x, self.building_y, 
                                 fill=False, edgecolor='black', linewidth=2))
            
            # 绘制走廊
            corridor_center_y = self.building_y / 2
            corridor_width = 2.0
            ax.add_patch(Rectangle((0, corridor_center_y - corridor_width/2), 
                                 self.building_x, corridor_width, 
                                 fill=True, color='lightgray', alpha=0.5, 
                                 label='走廊'))
            
            # 绘制教室
            for room in self.rooms:
                if room.floor == floor:
                    ax.add_patch(Rectangle((room.x, room.y), room.width, room.height, 
                                         fill=True, color='lightblue', alpha=0.3, 
                                         edgecolor='blue', linewidth=1))
                    ax.text(room.x + room.width/2, room.y + room.height/2, 
                           room.name, ha='center', va='center', fontsize=8)
            
            # 绘制AP位置
            ap_colors = ['red', 'green', 'blue', 'orange', 'purple', 'brown', 'pink', 'gray']
            for i, ap in enumerate(self.access_points):
                if ap.floor == floor + 1:
                    color = ap_colors[i % len(ap_colors)]
                    ax.scatter(ap.x, ap.y, s=200, c=color, marker='^', 
                              edgecolors='black', linewidth=1, zorder=5)
                    ax.text(ap.x, ap.y+2, f'AP{ap.id}\nCh{ap.channel_2_4}/{ap.channel_5}', 
                           ha='center', fontsize=7, fontweight='bold')
            
            ax.set_xlim(-5, self.building_x + 5)
            ax.set_ylim(-5, self.building_y + 5)
            ax.set_xlabel('X (m)')
            ax.set_ylabel('Y (m)')
            ax.set_title(f'楼层 {floor+1} - AP分布')
            ax.grid(True, alpha=0.3)
            ax.legend(loc='upper right')
            ax.set_aspect('equal')
        
        plt.tight_layout()
        
        if save_fig:
            plt.savefig('ap_distribution.png', dpi=300, bbox_inches='tight')
            print("已保存AP分布图: ap_distribution.png")
        
        plt.show()
        
        return fig
    
    def print_coverage_report(self):
        """打印覆盖报告"""
        print("\n" + "="*60)
        print("无线网络覆盖分析报告")
        print("="*60)
        
        print(f"\n建筑信息:")
        print(f"  - 尺寸: {self.building_x}m × {self.building_y}m × {self.building_z}层")
        print(f"  - 楼层高度: {self.floor_height}m")
        print(f"  - 网格精度: {self.grid_size}m")
        
        print(f"\nAP部署信息:")
        print(f"  - AP总数: {len(self.access_points)}")
        print(f"  - 每层AP数: {len(self.access_points) // self.building_z}")
        
        print("\nAP详细信息:")
        print("ID\t楼层\tX\tY\t2.4GHz信道\t5GHz信道")
        print("-"*50)
        for ap in self.access_points:
            print(f"{ap.id}\t{ap.floor}\t{ap.x:.1f}\t{ap.y:.1f}\t{ap.channel_2_4}\t\t{ap.channel_5}")
        
        print(f"\n覆盖统计:")
        print(f"  - 总网格点数: {self.coverage_stats['total_points']}")
        print(f"  - 2.4GHz覆盖点数: {self.coverage_stats['covered_2_4']} ({self.coverage_stats['coverage_rate_2_4']:.2f}%)")
        print(f"  - 5GHz覆盖点数: {self.coverage_stats['covered_5']} ({self.coverage_stats['coverage_rate_5']:.2f}%)")
        print(f"  - 双频覆盖点数: {self.coverage_stats['covered_both']} ({self.coverage_stats['coverage_rate_both']:.2f}%)")
        
        # 按楼层统计
        print(f"\n按楼层统计:")
        for floor in range(self.building_z):
            floor_points = 0
            floor_covered_2_4 = 0
            floor_covered_5 = 0
            
            for y in range(self.grid.shape[1]):
                for x in range(self.grid.shape[0]):
                    grid_point = self.grid[x, y, floor]
                    if grid_point:
                        floor_points += 1
                        if grid_point.covered_2_4:
                            floor_covered_2_4 += 1
                        if grid_point.covered_5:
                            floor_covered_5 += 1
            
            coverage_rate_2_4 = (floor_covered_2_4 / floor_points * 100) if floor_points > 0 else 0
            coverage_rate_5 = (floor_covered_5 / floor_points * 100) if floor_points > 0 else 0
            
            print(f"  楼层 {floor+1}: 2.4GHz覆盖{coverage_rate_2_4:.1f}%, 5GHz覆盖{coverage_rate_5:.1f}%")
        
        print("\n信道分配分析:")
        # 检查信道冲突
        channel_counts_2_4 = defaultdict(int)
        channel_counts_5 = defaultdict(int)
        
        for ap in self.access_points:
            channel_counts_2_4[ap.channel_2_4] += 1
            channel_counts_5[ap.channel_5] += 1
        
        print("  2.4GHz信道使用情况:")
        for channel, count in sorted(channel_counts_2_4.items()):
            print(f"    信道 {channel}: {count}个AP使用")
        
        print("\n  5GHz信道使用情况:")
        for channel, count in sorted(channel_counts_5.items()):
            print(f"    信道 {channel}: {count}个AP使用")
        
        # 检查相邻AP信道冲突
        print("\n信道冲突分析:")
        conflict_count = 0
        for i, ap1 in enumerate(self.access_points):
            for j, ap2 in enumerate(self.access_points[i+1:], i+1):
                distance = self.calculate_distance(ap1.x, ap1.y, ap1.z, ap2.x, ap2.y, ap2.z)
                if distance < 30:  # 30米内视为相邻
                    if ap1.channel_2_4 == ap2.channel_2_4:
                        print(f"  AP{ap1.id}和AP{ap2.id} 2.4GHz信道冲突: 信道{ap1.channel_2_4}")
                        conflict_count += 1
                    if ap1.channel_5 == ap2.channel_5:
                        print(f"  AP{ap1.id}和AP{ap2.id} 5GHz信道冲突: 信道{ap1.channel_5}")
                        conflict_count += 1
        
        if conflict_count == 0:
            print("  无信道冲突 ✓")
        
        print("="*60)
        
        return self.coverage_stats
    
    def export_to_csv(self, floor=0):
        """导出热力图数据到CSV"""
        if floor >= self.building_z:
            print(f"楼层 {floor} 不存在")
            return
        
        data = []
        
        for y_idx in range(self.grid.shape[1]):
            for x_idx in range(self.grid.shape[0]):
                grid_point = self.grid[x_idx, y_idx, floor]
                if grid_point:
                    data.append({
                        'X': grid_point.x,
                        'Y': grid_point.y,
                        'Signal_2_4': grid_point.signal_2_4,
                        'Signal_5': grid_point.signal_5,
                        'Covered_2_4': 1 if grid_point.covered_2_4 else 0,
                        'Covered_5': 1 if grid_point.covered_5 else 0,
                        'Best_AP': grid_point.best_ap_id,
                        'Wall_Type': grid_point.wall_type.name,
                        'In_Corridor': 1 if grid_point.in_corridor else 0,
                        'In_Room': 1 if grid_point.in_room else 0,
                        'Room_ID': grid_point.room_id
                    })
        
        df = pd.DataFrame(data)
        filename = f'floor_{floor+1}_coverage_data.csv'
        df.to_csv(filename, index=False, encoding='utf-8-sig')
        print(f"已导出数据到: {filename}")
        
        return df
    
    def export_ap_info(self):
        """导出AP信息到CSV"""
        data = []
        for ap in self.access_points:
            data.append({
                'ID': ap.id,
                'Floor': ap.floor,
                'X': ap.x,
                'Y': ap.y,
                'Z': ap.z,
                'Channel_2_4': ap.channel_2_4,
                'Channel_5': ap.channel_5,
                'Coverage_Radius_2_4': ap.coverage_radius_2_4,
                'Coverage_Radius_5': ap.coverage_radius_5,
                'TX_Power': ap.tx_power
            })
        
        df = pd.DataFrame(data)
        filename = 'ap_deployment_info.csv'
        df.to_csv(filename, index=False, encoding='utf-8-sig')
        print(f"已导出AP信息到: {filename}")
        
        return df

# 主程序
def main():
    """主函数"""
    print("="*60)
    print("教学楼无线网络AP部署模拟系统")
    print("="*60)
    
    # 1. 创建建筑
    print("\n1. 创建教学楼模型...")
    building = Building(
        building_x=BUILDING_X,
        building_y=BUILDING_Y,
        building_z=BUILDING_Z,
        floor_height=FLOOR_HEIGHT,
        grid_size=GRID_SIZE
    )
    
    # 2. 部署AP
    print("\n2. 部署无线AP...")
    aps_per_floor = 5
    building.deploy_aps_strategic(aps_per_floor)
    
    # 3. 计算初始覆盖
    print("\n3. 计算初始信号覆盖...")
    building.calculate_coverage()
    
    # 4. 显示初始覆盖报告
    print("\n4. 初始覆盖报告:")
    building.print_coverage_report()
    
    # 5. 优化AP位置
    print("\n5. 优化AP部署位置...")
    building.optimize_ap_positions(max_iterations=5)
    
    # 6. 显示优化后报告
    print("\n6. 优化后覆盖报告:")
    building.print_coverage_report()
    
    # 7. 可视化
    print("\n7. 生成可视化图表...")
    
    # 可视化AP分布
    building.visualize_ap_distribution()
    
    # 可视化每层热力图
    for floor in range(BUILDING_Z):
        print(f"\n生成楼层 {floor+1} 热力图...")
        building.visualize_heatmap(floor=floor, save_fig=True)
        building.visualize_3d_coverage(floor=floor, save_fig=True)
    
    # 8. 导出数据
    print("\n8. 导出数据...")
    for floor in range(BUILDING_Z):
        building.export_to_csv(floor=floor)
    
    building.export_ap_info()
    
    # 9. 生成汇总报告
    print("\n9. 生成汇总报告...")
    
    # 创建汇总报告文本文件
    with open('ap_deployment_summary.txt', 'w', encoding='utf-8') as f:
        f.write("="*60 + "\n")
        f.write("教学楼无线网络AP部署总结报告\n")
        f.write("="*60 + "\n\n")
        
        f.write(f"建筑信息:\n")
        f.write(f"  - 尺寸: {BUILDING_X}m × {BUILDING_Y}m × {BUILDING_Z}层\n")
        f.write(f"  - 楼层高度: {FLOOR_HEIGHT}m\n")
        f.write(f"  - 网格精度: {GRID_SIZE}m\n\n")
        
        f.write(f"AP部署信息:\n")
        f.write(f"  - AP总数: {len(building.access_points)}\n")
        f.write(f"  - 每层AP数: {aps_per_floor}\n\n")
        
        f.write("AP详细部署信息:\n")
        f.write("ID\t楼层\tX坐标\tY坐标\t2.4GHz信道\t5GHz信道\n")
        f.write("-"*50 + "\n")
        for ap in building.access_points:
            f.write(f"{ap.id}\t{ap.floor}\t{ap.x:.1f}\t{ap.y:.1f}\t{ap.channel_2_4}\t\t{ap.channel_5}\n")
        
        f.write(f"\n覆盖统计:\n")
        stats = building.coverage_stats
        f.write(f"  - 总网格点数: {stats['total_points']}\n")
        f.write(f"  - 2.4GHz覆盖: {stats['coverage_rate_2_4']:.2f}%\n")
        f.write(f"  - 5GHz覆盖: {stats['coverage_rate_5']:.2f}%\n")
        f.write(f"  - 双频覆盖: {stats['coverage_rate_both']:.2f}%\n\n")
        
        f.write("建议:\n")
        if stats['coverage_rate_both'] >= 95:
            f.write("  - ✓ 覆盖良好，无需调整\n")
        elif stats['coverage_rate_both'] >= 85:
            f.write("  - ○ 覆盖基本满足要求，可考虑优化\n")
        else:
            f.write("  - ✗ 覆盖不足，建议:\n")
            f.write("    1. 增加AP数量\n")
            f.write("    2. 调整AP位置\n")
            f.write("    3. 增加AP发射功率\n")
            f.write("    4. 使用定向天线\n")
        
        f.write("\n" + "="*60 + "\n")
    
    print("已生成汇总报告: ap_deployment_summary.txt")
    print("\n" + "="*60)
    print("AP部署模拟完成！")
    print("="*60)
    
    return building

# 运行主程序
if __name__ == "__main__":
    # 运行模拟
    building = main()
    
    # 交互式分析
    print("\n是否要进行交互式分析? (y/n)")
    user_input = input().strip().lower()
    
    if user_input == 'y':
        print("\n交互式分析选项:")
        print("1. 查看特定楼层热力图")
        print("2. 查看AP详细信息")
        print("3. 重新计算覆盖")
        print("4. 修改AP参数")
        print("5. 退出")
        
        while True:
            choice = input("\n请选择操作 (1-5): ").strip()
            
            if choice == '1':
                floor_num = int(input("请输入楼层号 (1-3): ")) - 1
                if 0 <= floor_num < BUILDING_Z:
                    building.visualize_heatmap(floor=floor_num, save_fig=False)
                else:
                    print("楼层号无效")
            
            elif choice == '2':
                print("\nAP详细信息:")
                for ap in building.access_points:
                    print(f"AP{ap.id}: 楼层{ap.floor}, 位置({ap.x:.1f}, {ap.y:.1f}), "
                          f"信道{ap.channel_2_4}/{ap.channel_5}")
            
            elif choice == '3':
                print("重新计算覆盖...")
                building.calculate_coverage()
                building.print_coverage_report()
            
            elif choice == '4':
                print("修改AP参数:")
                ap_id = int(input("请输入AP ID: "))
                ap_found = False
                for ap in building.access_points:
                    if ap.id == ap_id:
                        print(f"当前AP{ap_id}信息: 位置({ap.x}, {ap.y}), 信道{ap.channel_2_4}/{ap.channel_5}")
                        new_x = float(input("新X坐标(保持原值输入-1): "))
                        new_y = float(input("新Y坐标(保持原值输入-1): "))
                        new_channel_2_4 = int(input("新2.4GHz信道(保持原值输入-1): "))
                        new_channel_5 = int(input("新5GHz信道(保持原值输入-1): "))
                        
                        if new_x >= 0:
                            ap.x = new_x
                        if new_y >= 0:
                            ap.y = new_y
                        if new_channel_2_4 >= 0:
                            ap.channel_2_4 = new_channel_2_4
                        if new_channel_5 >= 0:
                            ap.channel_5 = new_channel_5
                        
                        print(f"AP{ap_id}已更新")
                        ap_found = True
                        break
                
                if not ap_found:
                    print(f"未找到AP{ap_id}")
                
                # 重新计算覆盖
                building.calculate_coverage()
                building.print_coverage_report()
            
            elif choice == '5':
                print("退出交互式分析")
                break
            
            else:
                print("无效选择，请重新输入")
    
    print("\n程序结束，所有结果已保存。")
