#pragma once

#include "ui_terrain_palette.h"

#include <QDialog>

#include "palette.h"
#include "terrain_brush.h"

import QRibbon;
import FlowLayout;

class TerrainPalette : public Palette {
	Q_OBJECT

public:
	TerrainPalette(QWidget* parent = nullptr);
	~TerrainPalette();

private:
	bool event(QEvent *e) override;

	Ui::TerrainPalette ui;
	QButtonGroup* textures_group = new QButtonGroup;
	FlowLayout* textures_layout = new FlowLayout;

	QButtonGroup* cliff_group = new QButtonGroup;
	FlowLayout* cliff_layout = new FlowLayout;

	TerrainBrush brush;
	QRibbonTab* ribbon_tab = new QRibbonTab;

public slots:
	void deactivate(QRibbonTab* tab) override;
};