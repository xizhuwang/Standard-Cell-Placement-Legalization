import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np
import os

def parse_aux_file(file_path):
    """
    解析 .aux 檔案，並識別相關的檔案，忽略 .pl 檔案。
    """
    files = {}
    with open(file_path, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith("#") or line == "":
                continue
            if ' : ' in line:
                tokens = line.split(' : ')
                if len(tokens) == 2:
                    _, file_list = tokens
                    parts = file_list.strip().split()
                    for part in parts:
                        if part.endswith('.nodes'):
                            files['nodes'] = part
                        elif part.endswith('.nets'):
                            files['nets'] = part
                        elif part.endswith('.wts'):
                            files['wts'] = part
                        elif part.endswith('.scl'):
                            files['scl'] = part
                        elif part.endswith('.route'):
                            files['route'] = part
                        elif part.endswith('.shapes'):
                            files['shapes'] = part
                        elif part.endswith('.sizes'):
                            files['sizes'] = part
                        elif part.endswith('.pl'):
                            # 忽略 .pl 檔案，因為我們將直接指定初始和最終的 .pl 檔案
                            continue
    return files

def parse_nodes_file(file_path):
    """
    解析 .nodes 檔案，獲取每個元件的尺寸及是否為終端元件。
    """
    components_sizes = {}
    components_terminal = {}
    with open(file_path, 'r') as f:
        header_skipped = False
        for line in f:
            line = line.strip()
            if line.startswith('#') or line == '':
                continue
            if not header_skipped:
                if line.startswith('UCLA nodes'):
                    continue
                elif 'NumNodes' in line or 'NumTerminals' in line:
                    continue
                else:
                    header_skipped = True
            parts = line.split()
            if len(parts) >= 3:
                component_name = parts[0]
                try:
                    width = float(parts[1])
                    height = float(parts[2])
                except ValueError:
                    print(f"Skipping invalid dimensions in {file_path}: {line}")
                    continue
                is_terminal = 'terminal' in parts
                components_sizes[component_name] = (width, height)
                components_terminal[component_name] = is_terminal
            else:
                print(f"Skipping invalid line in {file_path}: {line}")
    return components_sizes, components_terminal

def parse_pl_file(file_path):
    """
    解析 .pl 檔案，獲取每個元件的位置。
    """
    components = {}
    if not os.path.exists(file_path):
        print(f"Error: .pl file '{file_path}' does not exist.")
        return components
    with open(file_path, 'r') as f:
        header_skipped = False
        for line in f:
            line = line.strip()
            if line.startswith('#') or line == '':
                continue
            if not header_skipped:
                if line.startswith('UCLA pl'):
                    continue
                else:
                    header_skipped = True
            parts = line.split()
            if len(parts) >= 3:
                component_name = parts[0]
                try:
                    # 將數值轉換為 float 以支援小數，然後轉為 int
                    x_location = int(float(parts[1]))  
                    y_location = int(float(parts[2]))
                    components[component_name] = (x_location, y_location)
                except ValueError:
                    print(f"Skipping invalid coordinates in {file_path}: {line}")
            else:
                print(f"Skipping invalid line in {file_path}: {line}")
    return components


def parse_scl_file(file_path):
    """
    解析 .scl 檔案，獲取佈局行（rows）的資訊。
    返回包含每個 row 的詳細資訊的列表。
    每個 row 包含 x_start, y_start, width, height, site_width, height_unit。
    """
    rows = []
    current_row = {}
    if not os.path.exists(file_path):
        print(f"Error: .scl file '{file_path}' does not exist.")
        return rows
    with open(file_path, 'r') as f:
        header_skipped = False
        for line in f:
            line = line.strip()
            if line.startswith('#') or line == '':
                continue
            if not header_skipped:
                if line.startswith('UCLA scl'):
                    continue
                else:
                    header_skipped = True
            if line.startswith('CoreRow'):
                current_row = {}
            elif line.startswith('End'):
                if current_row:
                    required_keys = ['x_start', 'y_start', 'height', 'site_width', 'num_sites']
                    if all(k in current_row for k in required_keys):
                        width = current_row['num_sites'] * current_row['site_width']
                        height_unit = current_row['height']
                        rows.append((
                            int(current_row['x_start']),
                            int(current_row['y_start']),
                            int(width),
                            int(current_row['height']),
                            int(current_row['site_width']),
                            int(height_unit)
                        ))
                    else:
                        missing_keys = [k for k in required_keys if k not in current_row]
                        print(f"Missing keys {missing_keys} in row definition.")
                        print(f"Current row data: {current_row}")
            else:
                parts = line.replace(':', '').split()
                if len(parts) >= 2:
                    key = parts[0]
                    if key == 'Coordinate':
                        try:
                            current_row['y_start'] = int(float(parts[1]))
                        except (IndexError, ValueError):
                            print(f"Invalid Coordinate line: {line}")
                    elif key == 'Height':
                        try:
                            current_row['height'] = int(float(parts[1]))
                        except (IndexError, ValueError):
                            print(f"Invalid Height line: {line}")
                    elif key == 'Sitewidth':
                        try:
                            current_row['site_width'] = int(float(parts[1]))
                        except (IndexError, ValueError):
                            print(f"Invalid Sitewidth line: {line}")
                    elif key == 'SubrowOrigin':
                        try:
                            current_row['x_start'] = int(float(parts[1]))
                            current_row['num_sites'] = int(parts[3])
                        except (IndexError, ValueError):
                            print(f"Invalid SubrowOrigin line: {line}")
    return rows


def check_overlaps(components_rects, components_terminal):
    """
    檢查元件之間是否有重疊，忽略終端元件。
    """
    overlapping_pairs = []
    n = len(components_rects)
    for i in range(n):
        comp1, x1, y1, w1, h1 = components_rects[i]
        is_terminal1 = components_terminal.get(comp1, False)
        if is_terminal1:
            continue
        left1 = x1
        right1 = x1 + w1
        bottom1 = y1
        top1 = y1 + h1
        for j in range(i+1, n):
            comp2, x2, y2, w2, h2 = components_rects[j]
            is_terminal2 = components_terminal.get(comp2, False)
            if is_terminal2:
                continue
            left2 = x2
            right2 = x2 + w2
            bottom2 = y2
            top2 = y2 + h2
            if (left1 < right2 and right1 > left2 and
                bottom1 < top2 and top1 > bottom2):
                overlapping_pairs.append((comp1, comp2))
    return overlapping_pairs

def check_out_of_row_components(components_rects, scl_data, components_terminal):
    """
    檢查元件是否超出其所在行的範圍，忽略終端元件。
    """
    out_of_row_components = []
    for comp, x, y, w, h in components_rects:
        is_terminal = components_terminal.get(comp, False)
        if is_terminal:
            continue
        component_in_row = False
        left = x
        right = x + w
        bottom = y
        top = y + h
        for row in scl_data:
            row_x, row_y, row_w, row_h, site_width, height_unit = row
            row_left = row_x
            row_right = row_x + row_w
            row_bottom = row_y
            row_top = row_y + row_h
            if (left >= row_left and right <= row_right and
                bottom >= row_bottom and top <= row_top):
                component_in_row = True
                break
        if not component_in_row:
            out_of_row_components.append(comp)
    return out_of_row_components

def check_coordinate_alignment(components_rects, scl_data, components_terminal):
    """
    檢查每個元件的起始座標是否能被其所在行的 site_width 和 Height 整除。
    忽略終端元件。若不對齊，則記錄是 site 還是 row 不對齊。
    """
    misaligned_components = {}
    for comp, x, y, w, h in components_rects:
        is_terminal = components_terminal.get(comp, False)
        if is_terminal:
            continue

        # 找出元件所在的行
        row_found = False
        for row in scl_data:
            row_x, row_y, row_w, row_h, site_width, height_unit = row
            if y >= row_y and y + h <= row_y + row_h:
                row_found = True
                misalignment_reasons = []
                
                # 檢查 x 是否能被 site_width 整除
                if x % site_width != 0:
                    misalignment_reasons.append("site")
                
                # 檢查 y 是否位於該 row 的起始位置（y_start），或者能被 height_unit 整除
                if y != row_y and y % height_unit != 0:
                    misalignment_reasons.append("row")
                
                # 如果有未對齊原因，加入字典
                if misalignment_reasons:
                    misaligned_components[comp] = misalignment_reasons
                break

        if not row_found:
            # 如果找不到所在行，可能已經在 out_of_row_components 中處理
            continue

    return misaligned_components


def plot_components(pl_data_initial, pl_data_final, nodes_data, scl_data, overlapping_components, out_of_row_components, misaligned_components, components_terminal):
    """
    繪製佈局和元件移動的視覺化圖形。
    """
    fig, ax = plt.subplots(figsize=(12, 12))
    draw_layout(fig, ax, pl_data_initial, pl_data_final, nodes_data, scl_data, overlapping_components, out_of_row_components, misaligned_components, components_terminal)
    ax.set_title('Layout Visualization with Displacement Analysis')
    plt.show()

def draw_layout(fig, ax, pl_data_initial, pl_data_final, nodes_data, scl_data, overlapping_components, out_of_row_components, misaligned_components, components_terminal):
    """
    繪製行、元件及位移連線。
    """
    # 繪製行
    for row in scl_data:
        x, y, width, height, site_width, height_unit = row
        row_rect = patches.Rectangle((x, y), width, height, linewidth=1, edgecolor='blue', facecolor='lightblue', alpha=0.3)
        ax.add_patch(row_rect)

    # 建立初始和最終位置的元件矩形字典
    component_rects_initial = {}
    component_rects_final = {}
    for component, (x_initial, y_initial) in pl_data_initial.items():
        if component in nodes_data:
            width, height = nodes_data[component]
            component_rects_initial[component] = (x_initial, y_initial, width, height)
        else:
            print(f"Component {component} not found in nodes data (initial).")

    for component, (x_final, y_final) in pl_data_final.items():
        if component in nodes_data:
            width, height = nodes_data[component]
            component_rects_final[component] = (x_final, y_final, width, height)
        else:
            print(f"Component {component} not found in nodes data (final).")

    # 繪製元件和位移連線
    for component in pl_data_final:
        if component in nodes_data and component in pl_data_initial:
            x_initial, y_initial, width, height = component_rects_initial[component]
            x_final, y_final, _, _ = component_rects_final[component]
            is_terminal = components_terminal.get(component, False)

            # 根據元件狀態決定顏色
            if component in overlapping_components:
                edge_color = 'red'
                face_color = 'red'
                alpha = 0.5
            elif component in out_of_row_components:
                edge_color = 'orange'
                face_color = 'orange'
                alpha = 0.5
            elif component in misaligned_components:
                edge_color = 'cyan'  # 新增顏色表示座標未對齊
                face_color = 'cyan'
                alpha = 0.5
            elif is_terminal:
                edge_color = 'green'
                face_color = 'green'
                alpha = 0.5  # 用綠色表示 Terminal
            else:
                edge_color = 'purple'
                face_color = 'none'
                alpha = 1.0

            # 繪製最終位置的矩形
            rect = patches.Rectangle(
                (x_final, y_final), width, height, linewidth=1, edgecolor=edge_color, facecolor=face_color, alpha=alpha)
            ax.add_patch(rect)

            # 繪製從初始到最終位置的連線
            center_initial = (x_initial + width / 2, y_initial + height / 2)
            center_final = (x_final + width / 2, y_final + height / 2)
            ax.plot([center_initial[0], center_final[0]], [center_initial[1], center_final[1]], 
                    color='gray', linestyle='--', linewidth=0.5)
        else:
            print(f"Component {component} missing in initial or nodes data.")

    # 調整圖形的範圍
    all_x = []
    all_y = []
    for component in pl_data_final:
        if component in component_rects_final:
            x, y, width, height = component_rects_final[component]
            all_x.extend([x, x + width])
            all_y.extend([y, y + height])
    for row in scl_data:
        x, y, width, height, site_width, height_unit = row
        all_x.extend([x, x + width])
        all_y.extend([y, y + height])

    if all_x and all_y:
        min_x, max_x = min(all_x), max(all_x)
        min_y, max_y = min(all_y), max(all_y)

        # 添加邊界
        margin_x = (max_x - min_x) * 0.05  # 5% 邊距
        margin_y = (max_y - min_y) * 0.05
        ax.set_xlim([min_x - margin_x, max_x + margin_x])
        ax.set_ylim([min_y - margin_y, max_y + margin_y])
    else:
        ax.set_xlim([-35000, 30000])  # 無資料時的預設值
        ax.set_ylim([-35000, 30000])

    ax.set_aspect('equal', adjustable='box')
    ax.set_xlabel('X coordinate')
    ax.set_ylabel('Y coordinate')

    # 縮放功能
    def zoom(event):
        current_xlim = ax.get_xlim()
        current_ylim = ax.get_ylim()
        xdata = event.xdata
        ydata = event.ydata
        if xdata is None or ydata is None:
            return
        if event.button == 'up':
            scale_factor = 1 / 1.5
        elif event.button == 'down':
            scale_factor = 1.5
        else:
            return
        new_width = (current_xlim[1] - current_xlim[0]) * scale_factor
        new_height = (current_ylim[1] - current_ylim[0]) * scale_factor
        relx = (current_xlim[1] - xdata) / (current_xlim[1] - current_xlim[0])
        rely = (current_ylim[1] - ydata) / (current_ylim[1] - current_ylim[0])
        ax.set_xlim([xdata - new_width * (1 - relx), xdata + new_width * relx])
        ax.set_ylim([ydata - new_height * (1 - rely), ydata + new_height * rely])
        ax.figure.canvas.draw_idle()

    fig.canvas.mpl_connect('scroll_event', zoom)

    # 平移功能
    def on_press(event):
        if event.button == 1:
            ax._pan_start = event.x, event.y, ax.get_xlim(), ax.get_ylim()

    def on_motion(event):
        if hasattr(ax, '_pan_start') and event.button == 1:
            xpress, ypress, xlim, ylim = ax._pan_start
            dx = event.x - xpress
            dy = event.y - ypress
            scale_x = (xlim[1] - xlim[0]) / ax.bbox.width
            scale_y = (ylim[1] - ylim[0]) / ax.bbox.height
            ax.set_xlim(xlim[0] - dx * scale_x, xlim[1] - dx * scale_x)
            ax.set_ylim(ylim[0] - dy * scale_y, ylim[1] - dy * scale_y)
            ax.figure.canvas.draw_idle()

    def on_release(event):
        if event.button == 1 and hasattr(ax, '_pan_start'):
            del ax._pan_start

    fig.canvas.mpl_connect('button_press_event', on_press)
    fig.canvas.mpl_connect('motion_notify_event', on_motion)
    fig.canvas.mpl_connect('button_release_event', on_release)

def main():
    # 指定初始和最終的 .pl 檔案  這邊改檔案
    initial_pl = 'ibm05.pl'      
    final_pl = 'output02.pl'

    # 檢查初始和最終 .pl 檔案是否存在
    if not os.path.exists(initial_pl):
        print(f"Error: Initial .pl file '{initial_pl}' does not exist in the current directory.")
        return
    if not os.path.exists(final_pl):
        print(f"Error: Final .pl file '{final_pl}' does not exist in the current directory.")
        return

    aux_file = 'output02.aux'
    if not os.path.exists(aux_file):
        print(f"Error: .aux file '{aux_file}' does not exist in the current directory.")
        return

    files = parse_aux_file(aux_file)

    # 檢查必要的檔案是否存在
    try:
        nodes_file = files['nodes']
        scl_file = files['scl']
    except KeyError as e:
        print(f"Error: Missing {e.args[0]} file in the .aux file.")
        return

    # 解析資料
    pl_data_initial = parse_pl_file(initial_pl)
    pl_data_final = parse_pl_file(final_pl)
    nodes_data, components_terminal = parse_nodes_file(nodes_file)
    scl_data = parse_scl_file(scl_file)

    # 確認元件座標
    print("Sample component coordinates (Initial Positions):")
    sample_components_initial = list(pl_data_initial.keys())[:5]  # 列印前 5 個元件
    for component in sample_components_initial:
        x, y = pl_data_initial[component]
        print(f"Component {component}: x={x}, y={y}")

    print("\nSample component coordinates (Final Positions):")
    sample_components_final = list(pl_data_final.keys())[:5]  # 列印前 5 個元件
    for component in sample_components_final:
        x, y = pl_data_final[component]
        print(f"Component {component}: x={x}, y={y}")

    # 構建元件的矩形列表（僅使用最終位置進行重疊和超出行檢查）
    components_rects_final = []
    for component, (x, y) in pl_data_final.items():
        if component in nodes_data:
            width, height = nodes_data[component]
            components_rects_final.append((component, x, y, width, height))
        else:
            print(f"Component {component} not found in nodes data (final).")

    # 檢查元件之間的重疊
    overlaps = check_overlaps(components_rects_final, components_terminal)
    if overlaps:
        print("\n發現重疊的元件：")
        for comp1, comp2 in overlaps:
            print(f"{comp1} 與 {comp2} 發生重疊")
    else:
        print("\n沒有發現元件重疊。")

    # 建立重疊元件的集合
    overlapping_components = set()
    for comp1, comp2 in overlaps:
        overlapping_components.add(comp1)
        overlapping_components.add(comp2)

    # 檢查超出行範圍的元件
    out_of_row_components = check_out_of_row_components(components_rects_final, scl_data, components_terminal)
    if out_of_row_components:
        print("\n以下元件超出 row 的範圍：")
        for comp in out_of_row_components:
            print(f"{comp} 超出 row 的範圍")
    else:
        print("\n所有元件都在 row 的範圍內。")

    # 檢查座標對齊
    # 檢查座標對齊
    misaligned_components = check_coordinate_alignment(components_rects_final, scl_data, components_terminal)
    if misaligned_components:
        print("\n以下元件的座標未對齊 site 或 row：")
        for comp, reasons in misaligned_components.items():
            reasons_str = " 和 ".join(reasons)  # 格式化輸出
            print(f"{comp} 的座標未對齊 {reasons_str}")
    else:
        print("\n所有元件的座標都對齊 site_width 和 Height。")


    # 計算 Total Manhattan displacement 和 Maximum Manhattan displacement
    displacements = {}
    for component, (x_final, y_final) in pl_data_final.items():
        if component in pl_data_initial:
            x_initial, y_initial = pl_data_initial[component]
        else:
            x_initial, y_initial = 0, 0  # 由於座標為整數，假設為 (0,0)
            print(f"Warning: Component {component} 沒有在初始 .pl 檔案中找到，假設初始位置為 (0,0)")
        dx = x_final - x_initial
        dy = y_final - y_initial
        manhattan_displacement = abs(dx) + abs(dy)
        displacements[component] = manhattan_displacement

    total_manhattan_displacement = sum(displacements.values())
    if displacements:
        maximum_manhattan_displacement = max(displacements.values())
        max_manhattan_comp = max(displacements.items(), key=lambda x: x[1])[0]
    else:
        maximum_manhattan_displacement = 0
        max_manhattan_comp = None

    print(f"\nTotal Manhattan displacement: {total_manhattan_displacement}")
    if max_manhattan_comp:
        print(f"Maximum Manhattan displacement: {maximum_manhattan_displacement} (Component: {max_manhattan_comp})")
    else:
        print("No components to calculate maximum displacement.")

    # 視覺化佈局和位移分析
    plot_components(pl_data_initial, pl_data_final, nodes_data, scl_data, overlapping_components, out_of_row_components, misaligned_components, components_terminal)

if __name__ == "__main__":
    main()
